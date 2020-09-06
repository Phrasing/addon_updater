#ifndef PCH_H
#define PCH_H
#pragma once
// clang-format off

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <network/uri.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX
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
namespace rj = rapidjson;

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

#include "rapidjson_util.h"
#include "product_db.pb.h"

// clang-format off

#endif // !PCH_H


