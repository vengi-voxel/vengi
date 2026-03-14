/**
 * @file
 */

#include "HistogramTool.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

HistogramTool::HistogramTool() : Tool("voxedit_histogram") {
	_tool.set("description", "Get a color histogram for a model node. Returns the count and percentage of voxels for "
						   "each used palette color index, along with the RGBA color values.");
	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	json::Json _requiredArr = json::Json::array();
	_requiredArr.push("nodeUUID");
	inputSchema.set("required", _requiredArr);
	inputSchema.get("properties").set("nodeUUID", propUUID());
	_tool.set("inputSchema", core::move(inputSchema));
}

bool HistogramTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}

	scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraph().findNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node not found - fetch the scene state first", true);
	}

	if (!node->isAnyModelNode()) {
		return ctx.result(id, "Node is not a model node", true);
	}

	const core::DynamicArray<scenegraph::ColorHistogramEntry> &histogram = node->colorHistogram();
	const palette::Palette &palette = node->palette();

	json::Json result = json::Json::object();
	result.set("entries", json::Json::array());
	int totalVoxels = 0;

	for (int i = 0; i < (int)histogram.size(); i++) {
		const scenegraph::ColorHistogramEntry &entry = histogram[i];
		if (entry.count <= 0) {
			continue;
		}
		totalVoxels += entry.count;
		const color::RGBA &color = palette.color(entry.colorIndex);
		json::Json entryJson = json::Json::object();
		entryJson.set("colorIndex", (int)entry.colorIndex);
		entryJson.set("count", entry.count);
		entryJson.set("percentage", entry.percentage);
		entryJson.set("r", color.r);
		entryJson.set("g", color.g);
		entryJson.set("b", color.b);
		entryJson.set("a", color.a);
		if (!palette.colorName(entry.colorIndex).empty()) {
			entryJson.set("name", palette.colorName(entry.colorIndex).c_str());
		}
		result.get("entries").push(entryJson);
	}
	result.set("totalVoxels", totalVoxels);
	result.set("usedColors", (int)result.get("entries").size());

	return ctx.result(id, result.dump().c_str(), false);
}

} // namespace voxedit
