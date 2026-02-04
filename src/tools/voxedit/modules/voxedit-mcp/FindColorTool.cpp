/**
 * @file
 */

#include "FindColorTool.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

FindColorTool::FindColorTool() : Tool("voxedit_find_color") {
	_tool["description"] = "Find the closest matching color index in a node's palette for a given RGBA color.";

	nlohmann::json rProp = propTypeDescription("integer", "Red component (0-255)");
	rProp["minimum"] = 0;
	rProp["maximum"] = 255;
	nlohmann::json gProp = propTypeDescription("integer", "Green component (0-255)");
	gProp["minimum"] = 0;
	gProp["maximum"] = 255;
	nlohmann::json bProp = propTypeDescription("integer", "Blue component (0-255)");
	bProp["minimum"] = 0;
	bProp["maximum"] = 255;
	nlohmann::json aProp = propTypeDescription("integer", "Alpha component (0-255), defaults to 255");
	aProp["minimum"] = 0;
	aProp["maximum"] = 255;
	aProp["default"] = 255;

	nlohmann::json properties = nlohmann::json::object();
	properties["nodeUUID"] = propUUID();
	properties["r"] = core::move(rProp);
	properties["g"] = core::move(gProp);
	properties["b"] = core::move(bProp);
	properties["a"] = core::move(aProp);

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"nodeUUID", "r", "g", "b"});
	inputSchema["properties"] = core::move(properties);
	_tool["inputSchema"] = core::move(inputSchema);
}

bool FindColorTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
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

	const int r = args["r"].get<int>();
	const int g = args["g"].get<int>();
	const int b = args["b"].get<int>();
	const int a = args.value("a", 255);
	const color::RGBA rgba((uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a);

	const palette::Palette &palette = node->palette();
	const int matchIndex = palette.getClosestMatch(rgba);

	nlohmann::json resultJson;
	resultJson["colorIndex"] = matchIndex;
	if (matchIndex >= 0 && matchIndex < (int)palette.size()) {
		const color::RGBA &matchedColor = palette.color(matchIndex);
		resultJson["matchedColor"]["r"] = matchedColor.r;
		resultJson["matchedColor"]["g"] = matchedColor.g;
		resultJson["matchedColor"]["b"] = matchedColor.b;
		resultJson["matchedColor"]["a"] = matchedColor.a;
		if (!palette.colorName(matchIndex).empty()) {
			resultJson["matchedColor"]["name"] = palette.colorName(matchIndex).c_str();
		}
		const palette::Material &mat = palette.material(matchIndex);
		resultJson["matchedColor"]["material"] = nlohmann::json::array();
		for (int j = 0; j < (int)palette::MaterialProperty::MaterialMax; ++j) {
			const palette::MaterialProperty prop = (palette::MaterialProperty)j;
			if (mat.has(prop)) {
				resultJson["matchedColor"]["material"][palette::MaterialPropertyName(prop)] = mat.value(prop);
			}
		}
	}
	return ctx.result(id, resultJson.dump().c_str(), false);
}

} // namespace voxedit
