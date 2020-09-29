#ifndef RAPIDJSON_UTIL_H
#define RAPIDJSON_UTIL_H
#pragma once

#include  <addon_updater/rapidjson_util.inl>

namespace rj_util {

inline bool HasMemberOfType(const rj::Value::ConstObject& obj,
                            std::string_view member, const rj::Type type);
template <typename T>
inline std::optional<T> GetFieldIfExists(const rj::Value::ConstObject& obj,
                                         std::string_view member,
                                         const rj::Type type);

template <typename T>
inline T GetField(const rj::Value::ConstObject& obj, std::string_view member,
                  const rj::Type type, T def = T());

inline std::optional<std::string> GetString(const rj::Value::ConstObject& obj,
                                            std::string_view member);

inline std::optional<bool> GetBool(const rj::Value::ConstObject& obj,
                                   std::string_view member);

inline bool GetBoolDef(const rj::Value::ConstObject& obj,
                       std::string_view member, bool default_or = false);

inline std::string GetStringDef(const rj::Value::ConstObject& obj,
                                std::string_view member,
                                std::string_view default_or = "");
template <typename T = int32_t>
inline T ParseInt(const std::string& str);

}  // namespace rj_util

#endif  // !RAPIDJSON_UTIL_H
