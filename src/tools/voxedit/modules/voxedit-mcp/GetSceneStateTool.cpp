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
#define VALID_SKIPINFO_VALUES "palette, meshdetails, nodedetails, children, palettematerials, animations"
#define VALID_DETAIL_VALUES "summary, structure, full"

static uint32_t flagsForDetail(const core::String &detail) {
	if (detail == "summary") {
		// nodes, hierarchy, volume region and voxel counts - no palette/mesh/keyframes
		return scenegraph::JSONEXPORTER_CHILDREN | scenegraph::JSONEXPORTER_NODEDETAILS;
	}
	if (detail == "structure") {
		// summary plus keyframes and transforms
		return scenegraph::JSONEXPORTER_CHILDREN | scenegraph::JSONEXPORTER_NODEDETAILS |
			   scenegraph::JSONEXPORTER_ANIMATIONS;
	}
	if (detail == "full") {
		return scenegraph::JSONEXPORTER_ALL;
	}
	return 0u;
}

GetSceneStateTool::GetSceneStateTool() : Tool("voxedit_get_scene_state") {
	_tool.set("description",
			  "Get the current scene graph state. Call this first after connecting to get node UUIDs and structure. "
			  "Default detail is 'summary' (lean: nodes, sizes, voxel counts). Use 'structure' for keyframes/transforms, "
			  "'full' for palettes and mesh details. Prefer voxedit_screenshot for visuals and "
			  "voxedit_histogram/voxedit_get_palette for colors. If a node uuid is specified, only that node is returned.");
	json::Json inputSchema = objectSchema();
	inputSchema.get("properties").set("nodeUUID", propUUID());

	json::Json detailProp = propTypeDescription(
		"string", "Detail preset controlling output size. Default: summary. Valid: " VALID_DETAIL_VALUES
				  ". summary = nodes/sizes/voxel counts; structure = summary + keyframes/transforms; "
				  "full = everything including palettes and mesh details.");
	json::Json detailEnum = json::Json::array();
	detailEnum.push("summary");
	detailEnum.push("structure");
	detailEnum.push("full");
	detailProp.set("enum", core::move(detailEnum));
	detailProp.set("default", "summary");
	inputSchema.get("properties").set("detail", core::move(detailProp));

	inputSchema.get("properties").set(
		"skipinfo",
		propTypeDescription("string", "Comma separated list of things to omit from the json output on top of the "
									  "detail preset: " VALID_SKIPINFO_VALUES ". Useful to further reduce output size."));
	_tool.set("inputSchema", core::move(inputSchema));
}

bool GetSceneStateTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const scenegraph::SceneGraph &sceneGraph = ctx.sceneMgr->sceneGraph();
	if (sceneGraph.empty()) {
		return ctx.result(id, "Scene graph is empty - not connected or no scene loaded", true);
	}

	const core::String detail = args.strVal("detail", "summary");
	uint32_t flags = flagsForDetail(detail);
	if (flags == 0u) {
		return ctx.result(id, "Invalid detail - valid are: " VALID_DETAIL_VALUES, true);
	}

	if (args.contains("skipinfo")) {
		const core::String &skipInfoStr = args.strVal("skipinfo", "");
		core::DynamicArray<core::String> skipInfo;
		core::string::splitString(skipInfoStr, skipInfo, ",");
		for (const core::String &skipRaw : skipInfo) {
			const core::String skip = core::string::trim(skipRaw);
			if (skip.empty()) {
				continue;
			}
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
			} else if (skip == "animations") {
				flags &= ~scenegraph::JSONEXPORTER_ANIMATIONS;
			} else {
				return ctx.result(id, "Invalid skipinfo valid are: " VALID_SKIPINFO_VALUES, true);
			}
		}
	}

	io::BufferedReadWriteStream stream;
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
