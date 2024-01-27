/**
 * @file
 */

#include "JsonUtil.h"
#include "core/Log.h"

core::String get(nlohmann::json &json, const std::string &key, const core::String &defaultVal) {
	if (!json.contains(key)) {
		Log::debug("Missing key %s", key.c_str());
		return defaultVal;
	}
	return json[key].get<std::string>().c_str();
}

int getInt(nlohmann::json &json, const std::string &key, int defaultVal) {
	if (!json.contains(key)) {
		Log::debug("Missing key %s", key.c_str());
		return defaultVal;
	}
	return json[key].get<int>();
}
