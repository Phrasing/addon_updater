#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H
#pragma once
#include "singleton.h"

namespace addon_updater {

struct PairHash {
  template <class T1, class T2>
  std::size_t operator()(std::pair<T1, T2> const& pair) const {
    auto h1 = std::hash<T1>()(pair.first);
    auto h2 = std::hash<T1>()(pair.second);
    return h1 ^ h2;
  }
};

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
};

struct DownloadStatus {
  size_t content_size;
  size_t bytes_transferred;
  uint32_t progress;
  RequestState state = RequestState::kStateNone;
};

using LimitedTcpStream =
    beast::basic_stream<net::ip::tcp, net::any_io_executor,
                        beast::simple_rate_policy>;

using RequestCallback =
    std::function<void(const beast::error_code&, std::string_view)>;

using ProgressCallback = std::function<bool(const DownloadStatus&)>;

using DownloadCallback = std::function<void(
    const beast::error_code&, const DownloadStatus&, std::string_view)>;

using Parameters = std::vector<std::pair<std::string, std::string>>;

using Headers =
    std::unordered_set<std::pair<std::string, std::string>, PairHash>;

class AsyncHttpClient : public std::enable_shared_from_this<AsyncHttpClient> {
 public:
  explicit AsyncHttpClient(const net::any_io_executor& ex, ssl::context& ctx);
  ~AsyncHttpClient() = default;

  /*
    @request_callback - (const beast::error_code& ec, std::string_view body)
  */
  void Get(std::string_view url, const RequestCallback& request_callback,
           const Headers& headers = Headers());

  void Download(std::string_view url, const DownloadCallback& download_callback,
                const Headers& headers = Headers());

  void Download(std::string_view url, const RequestCallback& request_callback,
                const ProgressCallback& progress_callback,
                const Headers& headers = Headers());

  void Verbose(bool enable);

 private:
  void Resolve(beast::error_code ec,
               const tcp::resolver::results_type& results);
  void Connect(beast::error_code ec,
               const tcp::resolver::results_type::endpoint_type& endpoint);
  void Handshake(beast::error_code ec);
  void Write(beast::error_code ec, std::size_t bytes_transferred);
  void Read(beast::error_code ec, std::size_t bytes_transferred);
  void ReadHeader(beast::error_code ec, std::size_t bytes_transferred);
  void Shutdown(beast::error_code ec);
  void CallbackError(const beast::error_code& ec);
  void GetImpl(std::string_view url, const Headers& headers = Headers());

  DownloadCallback download_callback_;
  RequestCallback request_callback_;
  ProgressCallback progress_callback_;

  tcp::resolver resolver_;

  beast::ssl_stream<beast::tcp_stream> stream_;
  beast::flat_buffer buffer_;
  http::request<http::empty_body> req_;
  boost::optional<http::response_parser<http::string_body>> res_;

  std::string response_;
  size_t content_size_;
  size_t bytes_read_;

  bool verbose_enabled_;
};

class SyncHttpClient {
 public:
  SyncHttpClient(const net::any_io_executor& ex, ssl::context& ctx);
  ~SyncHttpClient() = default;

  HttpResponse Get(std::string_view url, const Headers& headers = Headers());

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
};

}  // namespace http_client

#endif  // !HTTP_CLIENT_H
