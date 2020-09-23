// clang-format off
#include "pch.h"
#include "http_client.h"

// clang-format on

namespace addon_updater {
namespace {
constexpr auto kHttpVersion = 11;
constexpr auto kSslPort = "443";
}  // namespace

AsyncHttpClient::AsyncHttpClient(const net::any_io_executor& ex,
                                 ssl::context& ctx)
    : resolver_(ex), stream_(ex, ctx), bytes_read_(0), content_size_(0) {
  res_.emplace();
  res_->body_limit(std::numeric_limits<size_t>::max());
}

void AsyncHttpClient::GetImpl(std::string_view url) {
  const auto uri = network::uri{url.data()};

  const auto host = std::string{uri.host().data(), uri.host().length()};
  const auto path = std::string{uri.path().data(), uri.path().length()};
  const auto query = std::string{uri.query().data(), uri.query().length()};

  if (!SSL_set_tlsext_host_name(stream_.native_handle(), host.c_str())) {
    this->Callback(beast::error_code{static_cast<int>(::ERR_get_error()),
                                     net::error::get_ssl_category()},
                   RequestState::kStateError);
    return;
  }

  req_.version(kHttpVersion);
  req_.method(http::verb::get);

  req_.target(path + "?" + query);
  req_.set(http::field::host, host);
  req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req_.set(http::field::proxy_connection, "Keep-Alive");

  if (verbose_enabled_) {
    std::cout << "\n[REQUEST]\n" << req_.base() << std::endl;
  }

  resolver_.async_resolve(
      host, kSslPort,
      beast::bind_front_handler(&AsyncHttpClient::Resolve,
                                std::move(shared_from_this())));
}

void AsyncHttpClient::Get(std::string_view url,
                          const RequestCallback& request_callback) {
  if (url.empty()) return;
  request_callback_ = request_callback;
  GetImpl(std::move(url));
}

void AsyncHttpClient::Download(std::string_view url,
                               const DownloadCallback& download_callback) {
  download_callback_ = (download_callback);
  GetImpl(std::move(url));
}

void AsyncHttpClient::Download(std::string_view url,
                               const RequestCallback& request_callback,
                               const ProgressCallback& progress_callback) {
  request_callback_ = request_callback;
  progress_callback_ = progress_callback;
  GetImpl(std::move(url));
}

void AsyncHttpClient::Verbose(bool enable) { verbose_enabled_ = enable; }

void AsyncHttpClient::Resolve(beast::error_code ec,
                              const tcp::resolver::results_type& results) {
  if (ec) {
    this->Callback(ec, RequestState::kStateError);
    return;
  }

  stream_.set_verify_mode(boost::asio::ssl::verify_none);
  beast::get_lowest_layer(stream_).async_connect(
      results, beast::bind_front_handler(&AsyncHttpClient::Connect,
                                         std::move(shared_from_this())));
}

void AsyncHttpClient::Connect(beast::error_code ec,
                              tcp::resolver::results_type::endpoint_type) {
  if (ec) {
    this->Callback(ec, RequestState::kStateError);
    return;
  }

  beast::get_lowest_layer(stream_).socket().set_option(
      net::ip::tcp::no_delay{true});
  stream_.async_handshake(
      boost::asio::ssl::stream_base::client,
      beast::bind_front_handler(&AsyncHttpClient::Handshake,
                                std::move(shared_from_this())));
}

void AsyncHttpClient::Handshake(beast::error_code ec) {
  if (ec) {
    this->Callback(ec, RequestState::kStateError);

    return;
  }

  http::async_write(stream_, req_,
                    beast::bind_front_handler(&AsyncHttpClient::Write,
                                              std::move(shared_from_this())));
}

void AsyncHttpClient::Write(beast::error_code ec,
                            std::size_t bytes_transferred) {
  if (ec) {
    this->Callback(ec, RequestState::kStateError);
    return;
  }

  boost::ignore_unused(bytes_transferred);
  http::async_read_header(
      stream_, buffer_, *res_,
      beast::bind_front_handler(&AsyncHttpClient::ReadHeader,
                                std::move(shared_from_this())));
}

void AsyncHttpClient::Read(beast::error_code ec,
                           std::size_t bytes_transferred) {
  auto progress = static_cast<uint32_t>((bytes_read_ += bytes_transferred) *
                                        (100.0F / content_size_));

  if (content_size_ > 0) {
    if (ec == boost::asio::error::eof) {
      progress = 100;
    }
  }

  if (!res_->is_done()) {
    if (!this->Callback(ec, RequestState::kStatePending, bytes_transferred,
                        progress)) {
      goto shutdown;
    }

    res_->release();
    http::async_read_some(
        stream_, buffer_, *res_,
        beast::bind_front_handler(&AsyncHttpClient::Read, shared_from_this()));

  } else {
  shutdown:
    stream_.async_shutdown(beast::bind_front_handler(&AsyncHttpClient::Shutdown,
                                                     shared_from_this()));

    if (ec && ec != boost::asio::error::eof) {
      this->Callback(ec, RequestState::kStateError, 0, 0);
    } else {
      ec = {};
      this->Callback(ec, RequestState::kStateFinish, bytes_transferred,
                     progress);
    }

    request_done_ = true;
  }
}

void AsyncHttpClient::ReadHeader(beast::error_code ec,
                                 std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);

  if (ec) {
    this->Callback(ec, RequestState::kStateError);
    return;
  }

  if (res_->content_length().is_initialized()) {
    content_size_ = res_->content_length().value();
  }

  this->Callback(ec, RequestState::kStatePending, bytes_transferred, 0);

  if (verbose_enabled_) {
    std::cout << "\n[HEADER]\n" << res_->get().base() << std::endl;
  }

  http::async_read_some(
      stream_, buffer_, *res_,
      beast::bind_front_handler(&AsyncHttpClient::Read, shared_from_this()));
}

void AsyncHttpClient::Shutdown(beast::error_code ec) {
  if (ec == boost::asio::error::eof) {
    ec = {};
  }
}

bool AsyncHttpClient::Callback(const beast::error_code& ec,
                               RequestState request_state,
                               size_t bytes_transferred, uint32_t progress) {
  if (request_callback_) {
    if (request_state == RequestState::kStatePending) {
      response_ += std::move(res_->get().body());
    } else if (request_state == RequestState::kStateFinish) {
      response_ += std::move(res_->get().body());
      request_callback_(ec, response_);
    }
  }

  if (download_callback_) {
    download_callback_(
        ec, {content_size_, bytes_transferred, progress, request_state},
        std::move(res_->get().body()));
  }

  if (progress_callback_) {
    if (!progress_callback_(
            {content_size_, bytes_transferred, progress, request_state}))
      return false;
  }

  return true;
}

ClientFactory::ClientFactory() : work_(ioc_), thd_pool_(4) {
  thd_ = std::thread([this] { ioc_.run(); });
}

ClientFactory::~ClientFactory() {
  ioc_.stop();
  thd_.join();
}

std::shared_ptr<AsyncHttpClient> ClientFactory::NewAsyncClient() {
  ssl::context ctx{ssl::context::tlsv12_client};
  ctx.set_verify_mode(ssl::verify_none);
  return std::make_shared<AsyncHttpClient>(net::make_strand(ioc_), ctx);
}

std::shared_ptr<SyncHttpClient> ClientFactory::NewSyncClient() {
  ssl::context ctx{ssl::context::tlsv12_client};
  ctx.set_verify_mode(ssl::verify_none);
  return std::make_shared<SyncHttpClient>(net::make_strand(ioc_), ctx);
}

SyncHttpClient::SyncHttpClient(const net::any_io_executor& ex,
                               ssl::context& ctx)
    : stream_(ex, ctx), resolver_(ex) {}

HttpResponse SyncHttpClient::Get(std::string_view url) {
  auto uri = network::uri{std::move(url.data())};

  const auto host = std::string{uri.host().data(), uri.host().length()};
  const auto path = std::string{uri.path().data(), uri.path().length()};
  const auto query = std::string{uri.query().data(), uri.query().length()};

  if (!SSL_set_tlsext_host_name(stream_.native_handle(), host.c_str())) {
    beast::error_code ec{static_cast<int>(::ERR_get_error()),
                         net::error::get_ssl_category()};
    return {"", ec};
  }

  beast::get_lowest_layer(stream_).connect(
      std::move(resolver_.resolve(host, kSslPort)));

  beast::get_lowest_layer(stream_).socket().set_option(
      net::ip::tcp::no_delay{true});

  stream_.handshake(ssl::stream_base::client);

  http::request<http::string_body> req{http::verb::get, path + "?" + query,
                                       kHttpVersion};

  req.set(http::field::host, host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req.set(http::field::proxy_connection, "Keep-Alive");

  http::write(stream_, req);

  std::optional<http::response_parser<http::string_body>> result;
  result.emplace();
  result->body_limit(std::numeric_limits<std::uint64_t>::max());

  beast::flat_buffer buffer;
  http::read(stream_, buffer, *result);

  beast::error_code ec;
  stream_.shutdown(ec);

  if (ec == net::error::eof) {
    ec = {};
  }

  defer { result->release(); };
  return {std::move(result->get().body()), ec};
}
void SyncHttpClient::Reset() {}
}  // namespace http_client
