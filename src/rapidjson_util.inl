#ifndef RAPIDJSON_UTIL_INL
#define RAPIDJSON_UTIL_INL
#pragma once

// clang-format off
#include "rapidjson_util.h"
// clang-format on

namespace rj_util {

inline bool HasMemberOfType(const rj::Value::ConstObject& obj,
                            const char* const member, const rj::Type type) {
  const auto result = obj.FindMember(member);
  return result != obj.MemberEnd() && result->value.GetType() == type;
}

template <typename T>
inline std::optional<T> GetFieldIfExists(const rj::Value::ConstObject& obj,
                                         const char* const member,
                                         const rj::Type type) {
  const auto result = obj.FindMember(member);
  if (result != obj.MemberEnd() && result->value.GetType() == type) {
    return result->value.Get<T>();
  }
  return std::nullopt;
}

template <typename T>
inline T GetField(const rj::Value::ConstObject& obj, const char* const member,  
                  const rj::Type type, T def) {
  return GetFieldIfExists<T>(obj, member, type).value_or(def);
}

inline std::optional<std::string> GetString(const rj::Value::ConstObject& obj,
                                            const char* const member) {
  return GetFieldIfExists<std::string>(obj, member, rj::kStringType);
}

inline std::optional<bool> GetBool(const rj::Value::ConstObject& obj,
                                   const char* const member) {
  const auto result = obj.FindMember(member);
  if (result != obj.MemberEnd() && result->value.GetType() == rj::kTrueType ||
      result->value.GetType() == rj::kFalseType) {
    return result->value.Get<bool>();
  }
  return std::nullopt;
}

inline bool GetBoolDef(const rj::Value::ConstObject& obj,
                       const char* const member, bool const defaultOr) {
  return GetBool(obj, member).value_or(defaultOr);
}

inline std::string GetStringDef(const rj::Value::ConstObject& obj,
                                const char* const member,
                                const char* const defaultOr) {
  return GetString(obj, member).value_or(defaultOr);
}

template <typename T = int32_t>
inline T ParseInt(const std::string& str) {
  return std::atoi(str.c_str());
}

}  // namespace rj_util

#endif  // !RAPIDJSON_UTIL_INL
