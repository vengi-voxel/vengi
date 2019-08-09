/**
 * @file
 */

#include "Rest.h"
#include "Var.h"
#include "GameConfig.h"

namespace core {

namespace rest {

core::rest::Response post(const std::string& url, const core::json& json) {
	core::rest::Connection conn(core::Var::getSafe(cfg::HTTPBaseURL)->strVal());
	conn.AppendHeader("Content-Type", "application/json");
	return conn.post(url, json.dump());
}

core::rest::Response get(const std::string& url) {
	core::rest::Connection conn(core::Var::getSafe(cfg::HTTPBaseURL)->strVal());
	return conn.get(url);
}

core::rest::Response put(const std::string& url, const core::json& json) {
	core::rest::Connection conn(core::Var::getSafe(cfg::HTTPBaseURL)->strVal());
	conn.AppendHeader("Content-Type", "application/json");
	return conn.put(url, json.dump());
}

}

}
