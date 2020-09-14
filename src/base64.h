#ifndef BEAST_BASE64_H
#define BEAST_BASE64_H
#pragma once

template <class = void>
inline std::string base64_encode(std::uint8_t const* data, std::size_t len) {
  std::string dest;
  dest.resize(beast::detail::base64::encoded_size(len));
  dest.resize(beast::detail::base64::encode(&dest[0], data, len));
  return dest;
}

inline std::string base64_encode(std::string const& s) {
  return base64_encode(reinterpret_cast<std::uint8_t const*>(s.data()),
                       s.size());
}

template <class = void>
inline std::string base64_decode(std::string const& data) {
  std::string dest;
  dest.resize(beast::detail::base64::decoded_size(data.size()));
  auto const result = beast::detail::base64::decode(&dest[0], data.data(), data.size());
  dest.resize(result.first);
  return dest;
}

#endif
