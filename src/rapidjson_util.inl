#ifndef RAPIDJSON_UTIL_INL
#define RAPIDJSON_UTIL_INL
#pragma once

// clang-format off
#include "rapidjson_util.h"
// clang-format on

namespace rj_util {

inline bool HasMemberOfType(const rj::Value::ConstObject& obj,
                            std::string_view member, const rj::Type type) {
  const auto result = obj.FindMember(std::move(member.data()));
  return result != obj.MemberEnd() && result->value.GetType() == type;
}

template <typename T>
inline std::optional<T> GetFieldIfExists(const rj::Value::ConstObject& obj,
                                         std::string_view member,
                                         const rj::Type type) {
  const auto result = obj.FindMember(std::move(member.data()));
  if (result != obj.MemberEnd() && result->value.GetType() == type) {
    return result->value.Get<T>();
  }
  return std::nullopt;
}

template <typename T>
inline T GetField(const rj::Value::ConstObject& obj, std::string_view member,
                  const rj::Type type, T def) {
  return GetFieldIfExists<T>(obj, member, type).value_or(def);
}

inline std::optional<std::string> GetString(const rj::Value::ConstObject& obj,
                                            std::string_view member) {
  return GetFieldIfExists<std::string>(obj, member, rj::Type::kStringType);
}

inline std::optional<bool> GetBool(const rj::Value::ConstObject& obj,
                                   std::string_view member) {
  const auto result = obj.FindMember(member.data());

  const auto is_bool_type = result->value.GetType() == rj::Type::kTrueType ||
                      result->value.GetType() == rj::Type::kFalseType;

  if (result != obj.MemberEnd() && is_bool_type) {
    return result->value.Get<bool>();
  }
  return std::nullopt;
}

inline bool GetBoolDef(const rj::Value::ConstObject& obj,
                       std::string_view member, bool default_or) {
  return GetBool(obj, member).value_or(default_or);
}

inline std::string GetStringDef(const rj::Value::ConstObject& obj,
                                std::string_view member,
                                std::string_view default_or) {
  return GetString(obj, member).value_or(default_or.data());
}

template <typename T>
inline T ParseInt(const std::string& str) {
  return static_cast<T>(std::atoi(str.c_str()));
}

}  // namespace rj_util

#endif  // !RAPIDJSON_UTIL_INL
