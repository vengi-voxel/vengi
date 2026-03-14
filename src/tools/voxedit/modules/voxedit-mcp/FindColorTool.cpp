/**
 * @file
 */

#include "FindColorTool.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

FindColorTool::FindColorTool() : Tool("voxedit_find_color") {
	_tool.set("description", "Find the closest matching color index in a node's palette for a given RGBA color.");

	json::Json rProp = propTypeDescription("integer", "Red");
	rProp.set("minimum", 0);
	rProp.set("maximum", 255);
	json::Json gProp = propTypeDescription("integer", "Green");
	gProp.set("minimum", 0);
	gProp.set("maximum", 255);
	json::Json bProp = propTypeDescription("integer", "Blue");
	bProp.set("minimum", 0);
	bProp.set("maximum", 255);
	json::Json aProp = propTypeDescription("integer", "Alpha");
	aProp.set("minimum", 0);
	aProp.set("maximum", 255);
	aProp.set("default", 255);

	json::Json properties = json::Json::object();
	properties.set("nodeUUID", propUUID());
	properties.set("r", core::move(rProp));
	properties.set("g", core::move(gProp));
	properties.set("b", core::move(bProp));
	properties.set("a", core::move(aProp));

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	json::Json _requiredArr = json::Json::array();
	_requiredArr.push("nodeUUID");
	_requiredArr.push("r");
	_requiredArr.push("g");
	_requiredArr.push("b");
	inputSchema.set("required", _requiredArr);
	inputSchema.set("properties", core::move(properties));
	_tool.set("inputSchema", core::move(inputSchema));
}

bool FindColorTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	if (!args.contains("r") || !args.contains("g") || !args.contains("b")) {
		return ctx.result(id, "Missing r, g, or b argument", true);
	}

	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}

	const scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraph().findNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node not found - fetch the scene state first", true);
	}

	const int r = args.get("r").intVal();
	const int g = args.get("g").intVal();
	const int b = args.get("b").intVal();
	const int a = args.intVal("a", 255);
	const color::RGBA rgba((uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a);

	const palette::Palette &palette = node->palette();
	const int matchIndex = palette.getClosestMatch(rgba);

	json::Json resultJson = json::Json::object();
	resultJson.set("colorIndex", matchIndex);
	if (matchIndex >= 0 && matchIndex < (int)palette.size()) {
		const color::RGBA &matchedColor = palette.color(matchIndex);
		resultJson.get("matchedColor").set("r", matchedColor.r);
		resultJson.get("matchedColor").set("g", matchedColor.g);
		resultJson.get("matchedColor").set("b", matchedColor.b);
		resultJson.get("matchedColor").set("a", matchedColor.a);
		if (!palette.colorName(matchIndex).empty()) {
			resultJson.get("matchedColor").set("name", palette.colorName(matchIndex).c_str());
		}
		const palette::Material &mat = palette.material(matchIndex);
		resultJson.get("matchedColor").set("material", json::Json::array());
		for (int j = 0; j < (int)palette::MaterialProperty::MaterialMax; ++j) {
			const palette::MaterialProperty prop = (palette::MaterialProperty)j;
			if (mat.has(prop)) {
				resultJson.get("matchedColor").get("material").set(palette::MaterialPropertyName(prop), mat.value(prop));
			}
		}
	}
	return ctx.result(id, resultJson.dump().c_str(), false);
}

} // namespace voxedit
