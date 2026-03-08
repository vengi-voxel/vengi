/**
 * @file
 */

#include "HistogramTool.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

HistogramTool::HistogramTool() : Tool("voxedit_histogram") {
	_tool["description"] = "Get a color histogram for a model node. Returns the count and percentage of voxels for "
						   "each used palette color index, along with the RGBA color values.";
	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"nodeUUID"});
	inputSchema["properties"]["nodeUUID"] = propUUID();
	_tool["inputSchema"] = core::move(inputSchema);
}

bool HistogramTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
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

	nlohmann::json result;
	result["entries"] = nlohmann::json::array();
	int totalVoxels = 0;

	for (int i = 0; i < (int)histogram.size(); i++) {
		const scenegraph::ColorHistogramEntry &entry = histogram[i];
		if (entry.count <= 0) {
			continue;
		}
		totalVoxels += entry.count;
		const color::RGBA &color = palette.color(entry.colorIndex);
		nlohmann::json entryJson;
		entryJson["colorIndex"] = (int)entry.colorIndex;
		entryJson["count"] = entry.count;
		entryJson["percentage"] = entry.percentage;
		entryJson["r"] = color.r;
		entryJson["g"] = color.g;
		entryJson["b"] = color.b;
		entryJson["a"] = color.a;
		if (!palette.colorName(entry.colorIndex).empty()) {
			entryJson["name"] = palette.colorName(entry.colorIndex).c_str();
		}
		result["entries"].emplace_back(core::move(entryJson));
	}
	result["totalVoxels"] = totalVoxels;
	result["usedColors"] = (int)result["entries"].size();

	return ctx.result(id, result.dump().c_str(), false);
}

} // namespace voxedit
