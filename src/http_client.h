#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H
#pragma once
#include "singleton.h"

namespace addon_updater {
enum class RequestState {
  kStatePending,
  kStateFinish,
  kStateError,
  kStateNone,
  kStateCancel
};

struct HttpResponse {
  std::string data;
  beast::error_code ec;
  bool Ok() { return !data.empty(); }
};

struct HttpField {
  http::field field_name;
  std::string field_value;
};

struct DownloadStatus {
  size_t content_size;
  size_t bytes_transferred;
  uint32_t progress;
  RequestState state = RequestState::kStateNone;
};

using LimitedTcpStream = beast::basic_stream<net::ip::tcp, net::any_io_executor,
                                             beast::simple_rate_policy>;

using RequestCallback =
    std::function<void(const beast::error_code&, std::string_view)>;

using ProgressCallback = std::function<bool(const DownloadStatus&)>;

using DownloadCallback = std::function<void(
    const beast::error_code&, const DownloadStatus&, std::string_view)>;

using RequestFields = std::vector<HttpField>;

class AsyncHttpClient : public std::enable_shared_from_this<AsyncHttpClient> {
 public:
  explicit AsyncHttpClient(const net::any_io_executor& ex, ssl::context& ctx);
  ~AsyncHttpClient() = default;

  AsyncHttpClient(const AsyncHttpClient&) = delete;
  AsyncHttpClient& operator=(const AsyncHttpClient&) = delete;
  AsyncHttpClient(AsyncHttpClient&&) = delete;
  AsyncHttpClient& operator=(AsyncHttpClient&&) = delete;

  void Get(std::string_view url, const RequestCallback& request_callback,
           const RequestFields& request_fields = {});

  void Download(std::string_view url, const DownloadCallback& download_callback,
                const RequestFields& request_fields = {});

  void Download(std::string_view url, const RequestCallback& request_callback,
                const ProgressCallback& progress_callback,
                const RequestFields& request_fields = {});

  void Verbose(bool enable);

  bool Finished() const { return request_done_; }

 private:
  void OnResolve(beast::error_code ec,
                 const tcp::resolver::results_type& results);

  void OnConnect(beast::error_code ec,
                 tcp::resolver::results_type::endpoint_type);

  void OnHandshake(beast::error_code ec);

  void OnWrite(beast::error_code ec, std::size_t bytes_transferred);

  void OnRead(beast::error_code ec, std::size_t bytes_transferred);

  void OnReadHeader(beast::error_code ec, std::size_t bytes_transferred);

  void OnShutdown(beast::error_code ec);

  bool Callback(const beast::error_code& ec, RequestState request_state,
                size_t bytes_transferred = 0u, uint32_t progress = 0u);

  void GetImpl(std::string_view url, const RequestFields& request_fields = {});

 private:
  DownloadCallback download_callback_;
  RequestCallback request_callback_;
  ProgressCallback progress_callback_;

  tcp::resolver resolver_;

  beast::ssl_stream<beast::tcp_stream> stream_;
  beast::flat_buffer buffer_;
  http::request<http::string_body> req_;
  boost::optional<http::response_parser<http::string_body>> res_;

  std::string response_;
  size_t content_size_;
  size_t bytes_read_;

  bool is_gzip_ = false;
  bool is_deflate_ = false;
  bool request_done_ = false;
  bool verbose_enabled_ = false;
};

class SyncHttpClient {
 public:
  SyncHttpClient(const net::any_io_executor& ex, ssl::context& ctx);
  ~SyncHttpClient() = default;

  HttpResponse Get(std::string_view url,
                   const RequestFields& request_fields = {});

  void Reset();

 private:
  beast::ssl_stream<beast::tcp_stream> stream_;
  tcp::resolver resolver_;
};

class ClientFactory : public Singleton<ClientFactory> {
 public:
  ClientFactory();
  ~ClientFactory();

  std::shared_ptr<AsyncHttpClient> NewAsyncClient();
  std::shared_ptr<SyncHttpClient> NewSyncClient();

  ClientFactory(const ClientFactory&) = delete;
  ClientFactory& operator=(const ClientFactory&) = delete;
  ClientFactory(ClientFactory&&) = delete;
  ClientFactory& operator=(ClientFactory&&) = delete;

 private:
  net::io_context ioc_;
  net::io_context::work work_;
  std::thread thd_;
  boost::asio::thread_pool thd_pool_;
  ssl::context ssl_context_;
};

}  // namespace http_client

#endif  // !HTTP_CLIENT_H
