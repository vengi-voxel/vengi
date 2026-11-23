/**
 * @file
 */

#include "SetPaletteColorTool.h"
#include "Authoring.h"
#include "color/ColorUtil.h"
#include "palette/Palette.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

SetPaletteColorTool::SetPaletteColorTool() : Tool("voxedit_set_palette_color") {
	_tool.set("description",
			  "Change the color of one palette slot. Every voxel using that slot changes with it. "
			  "index may be an existing slot, including 0, or colorCount to append one new slot. "
			  "nodeUUID defaults to the active model.");

	json::Json inputSchema = objectSchema();
	json::Json required = json::Json::array();
	required.push("index");
	required.push("color");
	inputSchema.set("required", required);
	json::Json properties = inputSchema.get("properties");
	json::Json nodeProp = propUUID();
	nodeProp.set("description", "Model node UUID. Defaults to the active model node.");
	properties.set("nodeUUID", nodeProp);
	json::Json index = propTypeDescription("integer", "Palette slot. 0 is valid here. colorCount appends a slot.");
	index.set("minimum", 0);
	index.set("maximum", 255);
	properties.set("index", index);
	properties.set("color", propTypeDescription("string", "#RRGGBB"));
	_tool.set("inputSchema", core::move(inputSchema));
}

bool SetPaletteColorTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	core::String error;
	scenegraph::SceneGraphNode *node = authoringNode(ctx, args, error);
	if (node == nullptr) {
		return ctx.result(id, error, true);
	}
	if (!args.contains("index") || !args.contains("color") || !args.get("color").isString()) {
		return ctx.result(id, "index and color (#RRGGBB) are required", true);
	}
	const int index = args.get("index").intVal();
	const int count = node->palette().colorCount();
	if (index < 0 || index > 255 || index > count || (index == count && count >= palette::PaletteMaxColors)) {
		return ctx.result(id, core::String::format("index must be 0..%i (existing slots) or %i to append",
												   core_max(0, count - 1), count),
						  true);
	}
	color::RGBA rgba;
	if (!color::parseColor(args.get("color").str(), rgba) || rgba.a == 0) {
		return ctx.result(id, "color must be #RRGGBB", true);
	}
	if (!ctx.sceneMgr->nodeSetColor(node->uuid(), (uint8_t)index, rgba)) {
		return ctx.result(id, "Failed to set the palette color", true);
	}
	return ctx.result(id, core::String::format("Palette slot %i is now %s.", index, color::toHex(rgba).c_str()), false);
}

} // namespace voxedit
