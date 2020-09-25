#ifndef PCH_H
#define PCH_H
#pragma once
// clang-format off

#include <IconsFontAwesome5.h>

#define MINIZ_HAS_64BIT_REGISTERS 1
#include <miniz.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <imgui_freetype.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION 
#define STB_IMAGE_WRITE_STATIC
#include <stbi_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION 
#define STB_IMAGE_RESIZE_STATIC
#include <stb_image_resize.h>


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

#ifdef GetObject
#undef GetObject  
#endif // !GetObject

#define GLEW_STATIC
#include <GL/glew.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <boost/algorithm/string_regex.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/beast/core/detail/base64.hpp>

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
#include <type_traits>
#include <vector>
#include <memory>
#include <cstdint>
#include <chrono>
#include <fstream>
#include <optional>
#include <filesystem>

#include "http_client.h"
#include "string_util.h"
#include "singleton.h"
#include "products.pb.h"
#include "defer.h"

// clang-format off

#endif // !PCH_H


