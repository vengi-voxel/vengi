/**
 * @file
 */

#include "DrawSlicesTool.h"
#include "Authoring.h"

namespace voxedit {

static constexpr int MaxSliceCells = 50000;

DrawSlicesTool::DrawSlicesTool() : Tool("voxedit_draw_slices") {
	_tool.set("description",
			  "Draw voxel art from text, one slice at a time. plane \"xz\" (default): each slice is one Y level "
			  "going up, each row is a Z line toward +Z, each character an X step. plane \"xy\": each slice is one "
			  "Z level toward +Z, rows go from the top down, characters along X. legend maps one character to "
			  "\"#RRGGBB\" or a palette index; -1 erases. '.' and space are always skipped. "
			  "nodeUUID defaults to the active model. Y is up.");

	json::Json inputSchema = objectSchema();
	json::Json required = json::Json::array();
	required.push("origin");
	required.push("slices");
	required.push("legend");
	inputSchema.set("required", required);

	json::Json properties = inputSchema.get("properties");
	json::Json nodeProp = propUUID();
	nodeProp.set("description", "Model node UUID. Defaults to the active model node.");
	properties.set("nodeUUID", nodeProp);
	properties.set("origin", propTypeDescription("array", "[x, y, z] of the first character of the first row of the first slice"));

	json::Json plane = propTypeDescription("string", "Slice plane. Default xz.");
	json::Json planeEnum = json::Json::array();
	planeEnum.push("xz");
	planeEnum.push("xy");
	plane.set("enum", planeEnum);
	plane.set("default", "xz");
	properties.set("plane", plane);

	json::Json slices = propTypeDescription("array", "List of slices. Each slice is a list of text rows.");
	json::Json sliceItems = json::Json::object();
	sliceItems.set("type", "array");
	json::Json rowItems = propTypeDescription("string", "One row of characters");
	sliceItems.set("items", rowItems);
	slices.set("items", sliceItems);
	properties.set("slices", slices);

	json::Json legend = propTypeDescription("object", "Character to color. Example: {\"#\": \"#8B5A2B\", \"g\": \"#3CB043\"}. Keys are one ASCII character. Values are #RRGGBB or a palette index; -1 erases.");
	properties.set("legend", legend);
	_tool.set("inputSchema", core::move(inputSchema));
}

bool DrawSlicesTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	core::String error;
	scenegraph::SceneGraphNode *node = authoringNode(ctx, args, error);
	if (node == nullptr) {
		return ctx.result(id, error, true);
	}
	if (!args.contains("origin") || !args.contains("slices") || !args.contains("legend")) {
		return ctx.result(id, "origin, slices and legend are required", true);
	}
	glm::ivec3 origin(0);
	if (!parseCell(args.get("origin"), origin, "origin", error)) {
		return ctx.result(id, error, true);
	}
	const json::Json legend = args.get("legend");
	if (!legend.isObject() || legend.empty()) {
		return ctx.result(id, "legend must map characters to colors, for example {\"#\": \"#8B5A2B\"}", true);
	}
	const json::Json slices = args.get("slices");
	if (!slices.isArray()) {
		return ctx.result(id, "slices must be a list of slices, each a list of text rows", true);
	}

	const bool planeXY = args.strVal("plane", "xz").toLower() == "xy";
	PaletteUse use;
	markUsedPaletteSlots(*node->volume(), use);
	bool legendErase[128];
	uint8_t legendIndex[128];
	bool legendSet[128];
	for (int i = 0; i < 128; ++i) {
		legendErase[i] = false;
		legendIndex[i] = 0;
		legendSet[i] = false;
	}
	bool approximated = false;
	for (auto it = legend.begin(); it != legend.end(); ++it) {
		const char *key = it.cKey();
		if (key == nullptr || key[0] == '\0' || key[1] != '\0' || (unsigned char)key[0] >= 128) {
			return ctx.result(id, core::String::format("Legend keys must be a single ASCII character, '%s' is not", key), true);
		}
		AuthoringColor color;
		if (!resolveAuthoringColor(ctx, *node, *it, use, color, error)) {
			return ctx.result(id, error, true);
		}
		const int ch = (unsigned char)key[0];
		legendSet[ch] = true;
		legendErase[ch] = color.erase;
		legendIndex[ch] = color.index;
		approximated = approximated || color.approximated;
	}

	core::DynamicArray<PaintCell> cells;
	bool skippedChar[128];
	for (int i = 0; i < 128; ++i) {
		skippedChar[i] = false;
	}
	for (int slice = 0; slice < slices.size(); ++slice) {
		const json::Json rows = slices.get(slice);
		if (!rows.isArray()) {
			return ctx.result(id, "Every slice must be a list of text rows", true);
		}
		for (int row = 0; row < rows.size(); ++row) {
			const json::Json rowJson = rows.get(row);
			if (!rowJson.isString()) {
				return ctx.result(id, "Every row must be a string", true);
			}
			const core::String text = rowJson.str();
			for (size_t column = 0; column < text.size(); ++column) {
				const unsigned char ch = (unsigned char)text[column];
				if (ch == ' ' || ch == '.') {
					continue;
				}
				if (ch >= 128 || !legendSet[ch]) {
					if (ch < 128) {
						skippedChar[ch] = true;
					}
					continue;
				}
				if ((int)cells.size() >= MaxSliceCells) {
					return ctx.result(id, "At most 50000 voxels per call - split the slices", true);
				}
				PaintCell cell;
				if (planeXY) {
					cell.pos = glm::ivec3(origin.x + (int)column, origin.y + (rows.size() - 1 - row), origin.z + slice);
				} else {
					cell.pos = glm::ivec3(origin.x + (int)column, origin.y + slice, origin.z + row);
				}
				cell.erase = legendErase[ch];
				cell.index = legendIndex[ch];
				cells.push_back(cell);
			}
		}
	}

	int changed = 0;
	int skipped = 0;
	if (!paintCells(ctx, *node, cells, changed, skipped)) {
		return ctx.result(id, "Volume not found - this is no model node", true);
	}
	core::String unknown;
	for (int ch = 0; ch < 128; ++ch) {
		if (skippedChar[ch]) {
			unknown += (char)ch;
			unknown += ' ';
		}
	}
	core::String message = core::String::format("Drew %i slice(s), %i voxels changed, %i outside the volume.", slices.size(),
												changed, skipped);
	if (!unknown.empty()) {
		message += " Characters not in the legend were skipped: ";
		message += unknown;
	}
	if (approximated) {
		message += " The palette was full, so a closest existing color was used.";
	}
	return ctx.result(id, message, false);
}

} // namespace voxedit
