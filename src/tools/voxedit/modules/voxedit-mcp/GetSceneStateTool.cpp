/**
 * @file
 */

#include "GetSceneStateTool.h"
#include "core/StringUtil.h"
#include "core/UUID.h"
#include "io/BufferedReadWriteStream.h"
#include "scenegraph/JsonExporter.h"
#include "scenegraph/SceneGraph.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

// map to JsonExporterFlags
#define VALID_SKIPINFO_VALUES "palette, meshdetails, nodedetails, children, palettematerials"

GetSceneStateTool::GetSceneStateTool() : Tool("voxedit_get_scene_state") {
	_tool["description"] =
		"Get the current scene graph state. This should be your first action after connecting to the MCP server to get "
		"the UUIDs of the existing nodes and their structure. Do this call from time to time to get an updated state. "
		"If a node uuid is specified, only a single node is returned.";
	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["properties"]["nodeUUID"] = propUUID();
	inputSchema["properties"]["skipinfo"] =
		propTypeDescription("string", "Comma separated list things to omit from the json output: " VALID_SKIPINFO_VALUES
									  ". Useful to reduce the output size if you only need a "
									  "subset of the information. By default, all details are included.");
	_tool["inputSchema"] = core::move(inputSchema);
}

bool GetSceneStateTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	const scenegraph::SceneGraph &sceneGraph = ctx.sceneMgr->sceneGraph();
	if (sceneGraph.empty()) {
		return ctx.result(id, "Scene graph is empty - not connected or no scene loaded", true);
	}

	io::BufferedReadWriteStream stream;
	uint32_t flags = scenegraph::JSONEXPORTER_ALL;
	if (args.contains("skipinfo")) {
		const core::String &skipInfoStr = args.value("skipinfo", "").c_str();
		core::DynamicArray<core::String> skipInfo;
		core::string::splitString(skipInfoStr, skipInfo, ",");
		for (const core::String &skip : skipInfo) {
			if (skip == "palette") {
				flags &= ~scenegraph::JSONEXPORTER_PALETTE;
			} else if (skip == "meshdetails") {
				flags &= ~scenegraph::JSONEXPORTER_MESHDETAILS;
			} else if (skip == "nodedetails") {
				flags &= ~scenegraph::JSONEXPORTER_NODEDETAILS;
			} else if (skip == "children") {
				flags &= ~scenegraph::JSONEXPORTER_CHILDREN;
			} else if (skip == "palettematerials") {
				flags &= ~scenegraph::JSONEXPORTER_PALETTEMATERIALS;
			} else {
				return ctx.result(id, "Invalid skipinfo valid are: " VALID_SKIPINFO_VALUES, true);
			}
		}
	}

	core::UUID nodeUUID = argsUUID(args);
	if (nodeUUID.isValid()) {
		if (const scenegraph::SceneGraphNode *node = sceneGraph.findNodeByUUID(nodeUUID)) {
			scenegraph::sceneGraphNodeJson(sceneGraph, node->id(), stream, flags);
		} else {
			scenegraph::sceneGraphJson(sceneGraph, stream, flags);
		}
	} else {
		scenegraph::sceneGraphJson(sceneGraph, stream, flags);
	}

	core::String json((const char *)stream.getBuffer(), (size_t)stream.size());
	return ctx.result(id, json, false);
}

} // namespace voxedit
