/**
 * @file
 */

#include "McpConfig.h"
#include "core/Log.h"
#include "core/Var.h"
#include "engine-config.h"
#include "voxedit-util/Config.h"

namespace voxedit {

static core::String mcpClientHostname(const core::String &iface) {
	if (iface.empty() || iface == "0.0.0.0" || iface == "::" || iface == "[::]") {
		return "127.0.0.1";
	}
	return iface;
}

void printMcpConfig(int port, const core::String &iface) {
	const core::String host = mcpClientHostname(iface);
	const core::String password = core::getVar(cfg::VoxEditNetPassword)->strVal();
	const core::String rconPassword = core::getVar(cfg::VoxEditNetRconPassword)->strVal();
	Log::printf(
		"{\n"
		"  \"mcpServers\": {\n"
		"    \"voxedit\": {\n"
		"      \"command\": \"%s-voxeditmcp\",\n"
		"      \"args\": [\"-set\", \"%s\", \"%s\", \"-set\", \"%s\", \"%i\", \"-set\", \"%s\", \"%s\", \"-set\", \"%s\", \"%s\"]\n"
		"    }\n"
		"  }\n"
		"}\n",
		PROJECT_NAME, cfg::VoxEditNetHostname, host.c_str(), cfg::VoxEditNetPort, port, cfg::VoxEditNetRconPassword,
		rconPassword.c_str(), cfg::VoxEditNetPassword, password.c_str());
}

} // namespace voxedit
