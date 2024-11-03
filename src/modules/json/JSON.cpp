/**
 * @file
 */

#include "JSON.h"

namespace json {

core::String toStr(const std::string &str) {
	core::String p(str.c_str());
	return p;
}

core::String toStr(const nlohmann::json &json, const char *key, const core::String &defaultValue) {
	return toStr(json.value(key, defaultValue.c_str()));
}

} // namespace json
