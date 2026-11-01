/**
 * @file
 */

#include "McpProtocol.h"

namespace voxedit {

static const char *const SupportedProtocolVersions[] = {"2024-11-05", "2025-03-26", "2025-06-18", "2025-11-25",
													   "2026-07-28"};

static const char *supportedVersionPointer(const core::String &asked) {
	for (const char *supported : SupportedProtocolVersions) {
		if (asked == supported) {
			return supported;
		}
	}
	return nullptr;
}

const char *mcpLatestProtocolVersion() {
	return "2026-07-28";
}

const char *mcpLatestLegacyProtocolVersion() {
	return "2025-11-25";
}

const char *mcpDefaultProtocolVersion() {
	return mcpLatestProtocolVersion();
}

bool mcpIsSupportedProtocolVersion(const core::String &version) {
	return supportedVersionPointer(version) != nullptr;
}

bool mcpIsModernProtocolVersion(const core::String &version) {
	return !version.empty() && version >= mcpLatestProtocolVersion();
}

const char *mcpNegotiateProtocolVersion(const core::String &asked) {
	if (const char *supported = supportedVersionPointer(asked)) {
		return supported;
	}
	return mcpLatestLegacyProtocolVersion();
}

const char *mcpServerInstructions() {
	return "This process is a network client of a running VoxEdit instance (ve_nethostname / ve_netport). "
		   "Call voxedit_get_scene_state first (detail=summary) to get node UUIDs; most edit tools require nodeUUID. "
		   "Block out with voxedit_shape_brush, refine with voxedit_place_voxels or the other brushes "
		   "(paint, line, plane, extrude, sculpt, transform, select), then voxedit_screenshot after each step. "
		   "Prefer screenshots over voxedit_get_voxels. "
		   "Read colors with voxedit_get_palette, voxedit_find_color, or voxedit_histogram. "
		   "Add models with voxedit_node_add_model. Undo/redo via voxedit_memento_*. "
		   "Lua scripts from the editor appear as voxedit_script_* tools; use voxedit_script_api and "
		   "voxedit_script_create to author them. Use voxedit_cmd_list / voxedit_cmd for editor commands. "
		   "Y is up.";
}

json::Json mcpSupportedProtocolVersions() {
	json::Json versions = json::Json::array();
	for (const char *version : SupportedProtocolVersions) {
		versions.push(version);
	}
	return versions;
}

json::Json mcpServerCapabilities() {
	json::Json capabilities = json::Json::object();
	json::Json tools = json::Json::object();
	tools.set("listChanged", true);
	capabilities.set("tools", tools);
	return capabilities;
}

json::Json mcpServerInfo(const core::String &name, const core::String &version) {
	json::Json serverInfo = json::Json::object();
	serverInfo.set("name", name);
	serverInfo.set("version", version);
	return serverInfo;
}

json::Json mcpRequestParams(const json::Json &request) {
	if (!request.contains("params")) {
		return json::Json();
	}
	return request.get("params");
}

json::Json mcpRequestMeta(const json::Json &request) {
	const json::Json params = mcpRequestParams(request);
	if (!params.isValid() || !params.isObject() || !params.contains("_meta")) {
		return json::Json();
	}
	return params.get("_meta");
}

bool mcpRequestHasModernMeta(const json::Json &request) {
	const json::Json meta = mcpRequestMeta(request);
	return meta.isValid() && meta.isObject() && meta.contains(McpMetaProtocolVersion);
}

bool mcpRequestHasClientCapabilities(const json::Json &request) {
	const json::Json meta = mcpRequestMeta(request);
	return meta.isValid() && meta.isObject() && meta.contains(McpMetaClientCapabilities);
}

core::String mcpRequestProtocolVersion(const json::Json &request) {
	const json::Json meta = mcpRequestMeta(request);
	if (meta.isValid() && meta.isObject() && meta.contains(McpMetaProtocolVersion)) {
		const json::Json version = meta.get(McpMetaProtocolVersion);
		if (version.isString()) {
			return version.str();
		}
	}
	const json::Json params = mcpRequestParams(request);
	if (params.isValid() && params.isObject() && params.contains("protocolVersion")) {
		const json::Json version = params.get("protocolVersion");
		if (version.isString()) {
			return version.str();
		}
	}
	return core::String();
}

json::Json mcpRequestClientInfo(const json::Json &request) {
	const json::Json meta = mcpRequestMeta(request);
	if (meta.isValid() && meta.isObject() && meta.contains(McpMetaClientInfo)) {
		return meta.get(McpMetaClientInfo);
	}
	const json::Json params = mcpRequestParams(request);
	if (params.isValid() && params.isObject() && params.contains("clientInfo")) {
		return params.get("clientInfo");
	}
	return json::Json();
}

void mcpApplyCompleteResult(json::Json &result, const core::String &serverName, const core::String &serverVersion) {
	result.set("resultType", "complete");
	json::Json meta = json::Json::object();
	if (result.contains("_meta") && result.get("_meta").isObject()) {
		meta = result.get("_meta");
	}
	meta.set(McpMetaServerInfo, mcpServerInfo(serverName, serverVersion));
	result.set("_meta", meta);
}

void mcpApplyCacheableResult(json::Json &result, int ttlMs, const char *cacheScope) {
	result.set("ttlMs", ttlMs);
	result.set("cacheScope", cacheScope);
}

json::Json mcpDiscoverResult(const core::String &serverName, const core::String &serverVersion) {
	json::Json result = json::Json::object();
	result.set("supportedVersions", mcpSupportedProtocolVersions());
	result.set("capabilities", mcpServerCapabilities());
	result.set("instructions", mcpServerInstructions());
	mcpApplyCacheableResult(result, 3600000, "public");
	mcpApplyCompleteResult(result, serverName, serverVersion);
	return result;
}

json::Json mcpInitializeResult(const char *protocolVersion, const core::String &serverName,
							   const core::String &serverVersion) {
	json::Json result = json::Json::object();
	result.set("protocolVersion", protocolVersion);
	result.set("capabilities", mcpServerCapabilities());
	result.set("serverInfo", mcpServerInfo(serverName, serverVersion));
	result.set("instructions", mcpServerInstructions());
	return result;
}

json::Json mcpUnsupportedProtocolVersionData(const core::String &requested) {
	json::Json data = json::Json::object();
	data.set("supported", mcpSupportedProtocolVersions());
	data.set("requested", requested);
	return data;
}

} // namespace voxedit
