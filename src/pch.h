#ifndef PCH_H
#define PCH_H
#pragma once
// clang-format off

#include <network/uri.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX

#include "product_db.pb.h"

#include <Windows.h>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;  // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

#include <unordered_set>
#include <iostream>
#include <string>
#include <thread>
#include <functional>
#include <vector>
#include <memory>
#include <cstdint>
#include <chrono>
#include <fstream>
#include <optional>
#include <filesystem>


// clang-format off

#endif // !PCH_H


