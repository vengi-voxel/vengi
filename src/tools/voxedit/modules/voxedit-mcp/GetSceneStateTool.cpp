/**
 * @file
 */

#include "GetSceneStateTool.h"
#include "io/BufferedReadWriteStream.h"
#include "scenegraph/JsonExporter.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

GetSceneStateTool::GetSceneStateTool() : Tool("voxedit_get_scene_state") {
	_tool["description"] =
		"Get the current scene graph state. This should be your first action after connecting to the MCP server to get "
		"the UUIDs of the existing nodes and their structure. Do this call from time to time to get an updated state.";
	_tool["inputSchema"]["type"] = "object";
}

bool GetSceneStateTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	if (ctx.sceneMgr->sceneGraph().empty()) {
		return ctx.result(id, "Scene graph is empty - not connected or no scene loaded", true);
	}
	io::BufferedReadWriteStream stream;
	scenegraph::sceneGraphJson(ctx.sceneMgr->sceneGraph(), stream);
	core::String json((const char *)stream.getBuffer(), (size_t)stream.size());
	return ctx.result(id, json, false);
}

} // namespace voxedit
