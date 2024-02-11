/**
 * @file
 */

#include "JsonUtil.h"
#include "core/Log.h"

core::String get(const nlohmann::json &json, const std::string &key, const core::String &defaultVal) {
	auto iter = json.find(key);
	if (iter != json.end()) {
		return iter->get<std::string>().c_str();
	}
	Log::debug("Missing key %s", key.c_str());
	return defaultVal;
}

int getInt(const nlohmann::json &json, const std::string &key, int defaultVal) {
	auto iter = json.find(key);
	if (iter != json.end()) {
		return iter->get<int>();
	}
	Log::debug("Missing key %s", key.c_str());
	return defaultVal;
}
