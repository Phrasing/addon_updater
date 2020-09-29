// clang-format off
#include <addon_updater/pch.h>
#include <addon_updater/http_client.h>
// clang-format on

namespace addon_updater {
namespace {
constexpr auto kHttpVersion = 11;
constexpr auto kSslPort = "443";
constexpr auto kUserAgent =
    R"(Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4183.121 Safari/537.36)";
constexpr auto kGzipField = "gzip";
constexpr auto kGzipAltField = "gzip-x";
constexpr auto kDeflateField = "deflate";
constexpr auto kMaxWBits = 15;

std::string GzipDecompress(std::string_view input) {
  std::string decompressed{};
  io::array_source src(input.data(), input.length());
  io::copy(io::compose(io::gzip_decompressor{}, std::move(src)),
           io::back_inserter(decompressed));
  return decompressed;
}

std::string zLibDecompress(std::string_view input) {
  std::string decompressed{};
  io::array_source src(input.data(), input.length());
  io::copy(io::compose(io::zlib_decompressor{-kMaxWBits}, std::move(src)),
           io::back_inserter(decompressed));
  return decompressed;
}

}  // namespace

AsyncHttpClient::AsyncHttpClient(const net::any_io_executor& ex,
                                 ssl::context& ctx)
    : resolver_(ex), stream_(ex, ctx), bytes_read_(0), content_size_(0) {}

void AsyncHttpClient::GetImpl(std::string_view url,
                              const RequestFields& request_fields) {
  auto encoded_url = skyr::url(url);

  if (!SSL_set_tlsext_host_name(stream_.native_handle(),
                                encoded_url.hostname().c_str())) {
    this->Callback(beast::error_code{static_cast<int>(::ERR_get_error()),
                                     net::error::get_ssl_category()},
                   RequestState::kStateError);
    return;
  }

  req_.version(kHttpVersion);
  req_.method(http::verb::get);

  req_.target(encoded_url.pathname() + encoded_url.search());
  req_.set(http::field::host, encoded_url.hostname());
  req_.set(http::field::user_agent, kUserAgent);

  for (const auto& field : request_fields) {
    req_.set(field.field_name, field.field_value);
  }

  if (verbose_enabled_) {
    std::cout << "\n[REQUEST]\n" << req_.base() << std::endl;
  }

  resolver_.async_resolve(
      encoded_url.hostname(), kSslPort,
      beast::bind_front_handler(std::move(&AsyncHttpClient::OnResolve),
                                shared_from_this()));
}

void AsyncHttpClient::Get(std::string_view url,
                          const RequestCallback& request_callback,
                          const RequestFields& request_fields) {
  if (url.empty()) return;
  request_callback_ = request_callback;
  GetImpl(std::move(url), request_fields);
}

void AsyncHttpClient::Download(std::string_view url,
                               const DownloadCallback& download_callback,
                               const RequestFields& request_fields) {
  download_callback_ = (download_callback);
  GetImpl(std::move(url), request_fields);
}

void AsyncHttpClient::Download(std::string_view url,
                               const RequestCallback& request_callback,
                               const ProgressCallback& progress_callback,
                               const RequestFields& request_fields) {
  request_callback_ = request_callback;
  progress_callback_ = progress_callback;
  GetImpl(std::move(url), request_fields);
}

void AsyncHttpClient::Verbose(bool enable) { verbose_enabled_ = enable; }

void AsyncHttpClient::OnResolve(beast::error_code ec,
                                const tcp::resolver::results_type& results) {
  if (ec) {
    this->Callback(ec, RequestState::kStateError);
    return;
  }

  stream_.set_verify_mode(boost::asio::ssl::verify_none);
  beast::get_lowest_layer(stream_).async_connect(
      results, beast::bind_front_handler(&AsyncHttpClient::OnConnect,
                                         shared_from_this()));
}

void AsyncHttpClient::OnConnect(beast::error_code ec,
                                tcp::resolver::results_type::endpoint_type) {
  if (ec) {
    this->Callback(ec, RequestState::kStateError);
    return;
  }

  beast::get_lowest_layer(stream_).socket().set_option(
      net::ip::tcp::no_delay{true});
  stream_.async_handshake(
      boost::asio::ssl::stream_base::client,
      beast::bind_front_handler(&AsyncHttpClient::OnHandshake,
                                shared_from_this()));
}

void AsyncHttpClient::OnHandshake(beast::error_code ec) {
  if (ec) {
    this->Callback(ec, RequestState::kStateError);
    return;
  }

  http::async_write(
      stream_, req_,
      beast::bind_front_handler(&AsyncHttpClient::OnWrite, shared_from_this()));
}

void AsyncHttpClient::OnWrite(beast::error_code ec,
                              std::size_t bytes_transferred) {
  if (ec) {
    this->Callback(ec, RequestState::kStateError);
    return;
  }

  res_.emplace();
  res_->body_limit(std::numeric_limits<uint64_t>::max());

  boost::ignore_unused(bytes_transferred);
  http::async_read_header(
      stream_, buffer_, *res_,
      beast::bind_front_handler(&AsyncHttpClient::OnReadHeader,
                                shared_from_this()));
}

void AsyncHttpClient::OnRead(beast::error_code ec,
                             std::size_t bytes_transferred) {
  auto progress =
      (content_size_ > 0 && ec == boost::asio::error::eof)
          ? 100
          : static_cast<uint32_t>((bytes_read_ += bytes_transferred) *
                                  (100.0F / content_size_));

  if (!res_->is_done()) {
    if (this->Callback(ec, RequestState::kStatePending, bytes_transferred,
                       progress)) {
      res_->release();
      http::async_read_some(stream_, buffer_, *res_,
                            beast::bind_front_handler(&AsyncHttpClient::OnRead,
                                                      shared_from_this()));
      return;
    }
  }

  stream_.async_shutdown(beast::bind_front_handler(&AsyncHttpClient::OnShutdown,
                                                   shared_from_this()));

  if (ec && ec != boost::asio::error::eof) {
    this->Callback(ec, RequestState::kStateError, 0, 0);
  } else {
    ec = {};
    this->Callback(ec, RequestState::kStateFinish, bytes_transferred, progress);
  }

  this->request_done_ = true;
}

void AsyncHttpClient::OnReadHeader(beast::error_code ec,
                                   std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);

  if (ec) {
    this->Callback(ec, RequestState::kStateError);
    return;
  }

  const auto& response = std::move(res_->get());

  if (res_->content_length().is_initialized()) {
    content_size_ = res_->content_length().value();
  }

  auto content_encoding = response[http::field::content_encoding];
  is_gzip_ =
      (content_encoding == kGzipField || content_encoding == kGzipAltField);
  is_deflate_ = (content_encoding == kDeflateField);

  this->Callback(ec, RequestState::kStatePending, bytes_transferred, 0);

  if (verbose_enabled_) {
    std::cout << "\n[HEADER]\n" << response.base() << std::endl;
  }

  http::async_read_some(
      stream_, buffer_, *res_,
      beast::bind_front_handler(&AsyncHttpClient::OnRead, shared_from_this()));
}

void AsyncHttpClient::OnShutdown(beast::error_code ec) {
  if (ec == boost::asio::error::eof) {
    ec = {};
  }
}

bool AsyncHttpClient::Callback(const beast::error_code& ec,
                               RequestState request_state,
                               size_t bytes_transferred, uint32_t progress) {
  const auto& response = std::move(res_->get());
  if (request_callback_) {
    response_ += response.body();
    if (request_state == RequestState::kStateFinish) {
      if (is_gzip_) {
        request_callback_(ec, std::move(GzipDecompress(this->response_)));
      } else if (is_deflate_) {
        request_callback_(ec, std::move(zLibDecompress(this->response_)));
      } else {
        request_callback_(ec, std::move(this->response_));
      }
    }
  }

  if (download_callback_) {
    if (is_gzip_) {
      download_callback_(
          ec, {content_size_, bytes_transferred, progress, request_state},
          std::move(GzipDecompress(response.body())));
    } else if (is_deflate_) {
      download_callback_(
          ec, {content_size_, bytes_transferred, progress, request_state},
          std::move(zLibDecompress(response.body())));
    } else {
      download_callback_(
          ec, {content_size_, bytes_transferred, progress, request_state},
          response.body());
    }
  }

  if (progress_callback_) {
    if (!progress_callback_(
            {content_size_, bytes_transferred, progress, request_state}))
      return false;
  }

  return true;
}

ClientFactory::ClientFactory()
    : work_(ioc_), ssl_context_(ssl::context::tlsv12_client) {
  ssl_context_.set_verify_mode(ssl::verify_none);
  thd_ = std::thread([this] { ioc_.run(); });
}

ClientFactory::~ClientFactory() {
  ioc_.stop();
  thd_.join();
}

std::shared_ptr<AsyncHttpClient> ClientFactory::NewAsyncClient() {
  return std::make_shared<AsyncHttpClient>(net::make_strand(ioc_),
                                           ssl_context_);
}

std::shared_ptr<SyncHttpClient> ClientFactory::NewSyncClient() {
  return std::make_shared<SyncHttpClient>(net::make_strand(ioc_), ssl_context_);
}

SyncHttpClient::SyncHttpClient(const net::any_io_executor& ex,
                               ssl::context& ctx)
    : stream_(ex, ctx), resolver_(ex) {}

HttpResponse SyncHttpClient::Get(std::string_view url,
                                 const RequestFields& request_fields) {
  auto encoded_url = skyr::url(url);

  if (!SSL_set_tlsext_host_name(stream_.native_handle(),
                                encoded_url.hostname().c_str())) {
    beast::error_code ec{static_cast<int>(::ERR_get_error()),
                         net::error::get_ssl_category()};
    return {"", ec};
  }

  beast::get_lowest_layer(stream_).connect(
      std::move(resolver_.resolve(encoded_url.hostname(), kSslPort)));

  beast::get_lowest_layer(stream_).socket().set_option(
      net::ip::tcp::no_delay{true});

  stream_.handshake(ssl::stream_base::client);

  http::request<http::string_body> req{
      http::verb::get, encoded_url.pathname() + encoded_url.search(),
      kHttpVersion};

  req.set(http::field::host, encoded_url.hostname());
  req.set(http::field::user_agent, kUserAgent);

  for (const auto& field : request_fields) {
    req.set(field.field_name, field.field_value);
  }

  http::write(stream_, req);

  http::response_parser<http::string_body> result;
  result.body_limit(std::numeric_limits<std::uint64_t>::max());
  defer { result.release(); };

  beast::flat_buffer buffer;
  http::read(stream_, buffer, result);

  beast::error_code ec;
  stream_.shutdown(ec);

  if (ec == net::error::eof) {
    ec = {};
  }

  const auto& response = std::move(result.get());

  if (response[http::field::content_encoding] == kGzipField ||
      response[http::field::content_encoding] == kGzipAltField) {
    return {std::move(GzipDecompress(response.body())), ec};
  }

  if (response[http::field::content_encoding] == kDeflateField) {
    return {std::move(zLibDecompress(response.body())), ec};
  }

  return {std::move(response.body()), ec};
}
void SyncHttpClient::Reset() {}
}  // namespace http_client
