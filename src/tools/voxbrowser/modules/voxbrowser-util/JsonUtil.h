/**
 * @file
 */

#pragma once

#include "core/String.h"
#include <json.hpp>

core::String get(nlohmann::json &json, const std::string &key, const core::String &defaultVal = "");
int getInt(nlohmann::json &json, const std::string &key, int defaultVal = 0);
