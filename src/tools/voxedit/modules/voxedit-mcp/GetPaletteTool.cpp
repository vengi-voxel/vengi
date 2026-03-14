/**
 * @file
 */

#include "GetPaletteTool.h"
#include "palette/Material.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

GetPaletteTool::GetPaletteTool() : Tool("voxedit_get_palette") {
	_tool.set("description", "Get the color palette of a specific node.");
	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	json::Json _requiredArr = json::Json::array();
	_requiredArr.push("nodeUUID");
	inputSchema.set("required", _requiredArr);
	inputSchema.get("properties").set("nodeUUID", propUUID());
	_tool.set("inputSchema", core::move(inputSchema));
}

bool GetPaletteTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}

	scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraph().findNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node not found - fetch the scene state first", true);
	}

	const palette::Palette &palette = node->palette();
	json::Json paletteJson = json::Json::object();
	paletteJson.set("name", palette.name().c_str());
	paletteJson.set("colorCount", palette.colorCount());
	paletteJson.set("colors", json::Json::array());
	for (size_t i = 0; i < palette.size(); ++i) {
		const color::RGBA &color = palette.color(i);
		json::Json colorJson = json::Json::object();
		colorJson.set("index", (int)i);
		colorJson.set("r", color.r);
		colorJson.set("g", color.g);
		colorJson.set("b", color.b);
		colorJson.set("a", color.a);
		if (!palette.colorName(i).empty()) {
			colorJson.set("name", palette.colorName(i).c_str());
		}
		const palette::Material &mat = palette.material(i);
		colorJson.set("material", json::Json::array());
		for (int j = 0; j < (int)palette::MaterialProperty::MaterialMax; ++j) {
			const palette::MaterialProperty prop = (palette::MaterialProperty)j;
			if (mat.has(prop)) {
				json::Json matEntry = json::Json::object();
				matEntry.set(palette::MaterialPropertyName(prop), mat.value(prop));
				colorJson.get("material").push(matEntry);
			}
		}
		paletteJson.get("colors").push(colorJson);
	}
	return ctx.result(id, paletteJson.dump().c_str(), false);
}

} // namespace voxedit
