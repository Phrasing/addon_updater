// clang-format off
#include "pch.h"
#include "http_client.h"

// clang-format on

namespace http_client {
namespace {
constexpr auto kHttpVersion = 11;
constexpr auto kSslPort = "443";
}  // namespace

AsyncHttpClient::AsyncHttpClient(const net::any_io_executor& ex,
                                 ssl::context& ctx)
    : resolver_(ex),
      stream_(ex, ctx),
      bytes_read_(0),
      content_size_(0),
      verbose_enabled_(false) {
  res_.emplace();
  (*res_).body_limit(std::numeric_limits<std::uint64_t>::max());
}

void AsyncHttpClient::GetImpl(std::string_view url, const Headers& headers) {
  const auto uri = network::uri{url.data()};

  const auto host = std::string{uri.host().data(), uri.host().length()};
  const auto path = std::string{uri.path().data(), uri.path().length()};
  const auto query = std::string{uri.query().data(), uri.query().length()};

  if (!SSL_set_tlsext_host_name(stream_.native_handle(), host.c_str())) {
    beast::error_code ec{static_cast<int>(::ERR_get_error()),
                         net::error::get_ssl_category()};
    std::cerr << ec.message() << "\n";
    return;
  }

  req_.version(kHttpVersion);
  req_.method(http::verb::get);

  req_.target(path + "?" + query);
  req_.set(http::field::host, host);
  req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  for (auto& header : headers) {
    req_.set(http::string_to_field(header.first), header.second);
  }

  if (verbose_enabled_) {
    std::cout << "\n[REQUEST]\n" << req_.base() << std::endl;
  }

  resolver_.async_resolve(
      host, kSslPort,
      beast::bind_front_handler(&AsyncHttpClient::Resolve, shared_from_this()));
}

void AsyncHttpClient::Get(std::string_view url,
                          const RequestCallback& request_callback,
                          const Headers& headers) {
  request_callback_ = request_callback;
  GetImpl(std::move(url), headers);
}

void AsyncHttpClient::Download(std::string_view url,
                               const DownloadCallback& download_callback,
                               const Headers& headers) {
  download_callback_ = (download_callback);
  GetImpl(std::move(url), headers);
}

void AsyncHttpClient::Download(std::string_view url,
                               const RequestCallback& request_callback,
                               const ProgressCallback& progress_callback,
                               const Headers& headers) {
  request_callback_ = (request_callback);
  progress_callback_ = (progress_callback);
  GetImpl(std::move(url), headers);
}

void AsyncHttpClient::Verbose(bool enable) { verbose_enabled_ = enable; }

void AsyncHttpClient::Resolve(beast::error_code ec,
                              tcp::resolver::results_type results) {
  stream_.set_verify_mode(boost::asio::ssl::verify_none);
  beast::get_lowest_layer(stream_).async_connect(
      results,
      beast::bind_front_handler(&AsyncHttpClient::Connect, shared_from_this()));
}

void AsyncHttpClient::Connect(beast::error_code ec,
                              tcp::resolver::results_type::endpoint_type) {
  tcp::no_delay option(true);
  stream_.next_layer().socket().set_option(option);
  stream_.async_handshake(boost::asio::ssl::stream_base::client,
                          beast::bind_front_handler(&AsyncHttpClient::Handshake,
                                                    shared_from_this()));
}

void AsyncHttpClient::Handshake(beast::error_code ec) {
  boost::beast::http::async_write(
      stream_, req_,
      beast::bind_front_handler(&AsyncHttpClient::Write, shared_from_this()));
}

void AsyncHttpClient::Write(beast::error_code ec,
                            std::size_t bytes_transferred) {
  boost::beast::http::async_read_header(
      stream_, buffer_, (*res_),
      beast::bind_front_handler(&AsyncHttpClient::ReadHeader,
                                shared_from_this()));
}

void AsyncHttpClient::Read(beast::error_code ec,
                           std::size_t bytes_transferred) {
  auto percent = static_cast<unsigned int>((bytes_read_ += bytes_transferred) *
                                           100.0f / content_size_);

  if (content_size_ > 0) {
    if (ec == boost::asio::error::eof) {
      percent = 100;
    }
  }

  auto callback = [this, percent, ec](RequestState state) {
    if (request_callback_) {
      request_callback_(ec, response_);
    } else if (download_callback_) {
      download_callback_(ec, percent, (*res_).get().body(),
                         RequestState::kStateFinish);
    }
    if (progress_callback_) {
      progress_callback_(percent, state);
    }
  };

  if (!(*res_).is_done()) {
    if (!download_callback_) {
      response_ += (*res_).get().body();
    } else {
      download_callback_(ec, percent, (*res_).get().body(),
                         RequestState::kStatePending);
    }

    if (progress_callback_) {
      progress_callback_(percent, RequestState::kStatePending);
    }

    (*res_).release();
    boost::beast::http::async_read_some(
        stream_, buffer_, (*res_),
        beast::bind_front_handler(&AsyncHttpClient::Read, shared_from_this()));

  } else {
    callback(RequestState::kStateFinish);

    stream_.async_shutdown(beast::bind_front_handler(&AsyncHttpClient::Shutdown,
                                                     shared_from_this()));

    if (ec && ec != boost::system::errc::not_connected) {
      callback(RequestState::kStateError);
    }
  }
}

void AsyncHttpClient::ReadHeader(beast::error_code ec,
                                 std::size_t bytes_transferred) {
  if (!(*res_).chunked()) {
    content_size_ = (*res_).content_length().is_initialized()
                        ? (*res_).content_length().value()
                        : 0;
  }

  if (verbose_enabled_) {
    std::cout << "\n[HEADER]\n" << (*res_).get().base() << std::endl;
  }

  boost::beast::http::async_read_some(
      stream_, buffer_, (*res_),
      beast::bind_front_handler(&AsyncHttpClient::Read, shared_from_this()));
}

void AsyncHttpClient::Shutdown(beast::error_code ec) {
  if (ec == boost::asio::error::eof) {
    ec.assign(0, ec.category());
  }
}

ClientFactory::ClientFactory() : work_(ioc_) {
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

HttpResponse SyncHttpClient::Get(std::string_view url,
                                 const Parameters& parameters,
                                 const Headers& headers) {
  auto uri = network::uri{url.data()};
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

  tcp::no_delay option(true);
  stream_.next_layer().socket().set_option(option);
  stream_.handshake(ssl::stream_base::client);

  http::request<http::string_body> req{http::verb::get, path + "?" + query,
                                       kHttpVersion};
  req.set(http::field::host, host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  for (auto& header : headers) {
    req.set(http::string_to_field(header.first), header.second);
  }

  http::write(stream_, req);

  boost::optional<http::response_parser<http::string_body>> result;
  result.emplace();
  (*result).body_limit(std::numeric_limits<std::uint64_t>::max());

  beast::flat_buffer buffer;
  http::read(stream_, buffer, (*result));

  beast::error_code ec;
  stream_.shutdown(ec);

  if (ec == net::error::eof) {
    ec = {};
  }

  return {(*result).get().body(), ec};
}
}  // namespace http_client
