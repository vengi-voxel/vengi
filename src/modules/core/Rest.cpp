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

}

}
