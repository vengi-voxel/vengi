/**
 * @file
 */

#include "GetPaletteTool.h"
#include "palette/Material.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

GetPaletteTool::GetPaletteTool() : Tool("voxedit_get_palette") {
	_tool["description"] = "Get the color palette of a specific node.";
	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"nodeUUID"});
	inputSchema["properties"]["nodeUUID"] = propUUID();
	_tool["inputSchema"] = core::move(inputSchema);
}

bool GetPaletteTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID", true);
	}

	scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraph().findNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node not found", true);
	}

	const palette::Palette &palette = node->palette();
	nlohmann::json paletteJson;
	paletteJson["name"] = palette.name().c_str();
	paletteJson["colorCount"] = palette.colorCount();
	paletteJson["colors"] = nlohmann::json::array();
	for (size_t i = 0; i < palette.size(); ++i) {
		const color::RGBA &color = palette.color(i);
		nlohmann::json colorJson;
		colorJson["index"] = (int)i;
		colorJson["r"] = color.r;
		colorJson["g"] = color.g;
		colorJson["b"] = color.b;
		colorJson["a"] = color.a;
		if (!palette.colorName(i).empty()) {
			colorJson["name"] = palette.colorName(i).c_str();
		}
		const palette::Material &mat = palette.material(i);
		colorJson["material"] = nlohmann::json::array();
		for (int j = 0; j < (int)palette::MaterialProperty::MaterialMax; ++j) {
			const palette::MaterialProperty prop = (palette::MaterialProperty)j;
			if (mat.has(prop)) {
				colorJson["material"][palette::MaterialPropertyName(prop)] = mat.value(prop);
			}
		}
		paletteJson["colors"].emplace_back(core::move(colorJson));
	}
	return ctx.result(id, paletteJson.dump().c_str(), false);
}

} // namespace voxedit
