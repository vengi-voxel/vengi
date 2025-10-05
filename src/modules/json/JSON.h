/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/Assert.h"

#define JSON_THROW_USER(exception) core_assert_msg(false, "%s", exception.what()); std::abort()
#define JSON_NO_IO
#include "private/json.hpp"

namespace json {
core::String toStr(const nlohmann::json &json, const char *key, const core::String &defaultValue = "");
core::String toStr(const std::string &str);
} // namespace json
