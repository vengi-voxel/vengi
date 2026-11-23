/**
 * @file
 */

#include "PlaceVoxelsTool.h"
#include "Authoring.h"
#include "palette/Palette.h"

namespace voxedit {

static constexpr int MaxPlaceVoxels = 50000;

PlaceVoxelsTool::PlaceVoxelsTool() : Tool("voxedit_place_voxels") {
	_tool.set("description",
			  "Place or erase voxels in a model node. Each entry is {x, y, z, idx}, {x, y, z, color} or "
			  "[x, y, z, color]. color and idx are a palette index or #RRGGBB; -1 erases. "
			  "A new hex color is added to the palette. "
			  "nodeUUID defaults to the active model. Y is up.");

	json::Json inputSchema = objectSchema();
	json::Json required = json::Json::array();
	required.push("voxels");
	inputSchema.set("required", required);
	json::Json properties = inputSchema.get("properties");
	properties.set("voxels", propVoxels());
	json::Json nodeProp = propUUID();
	nodeProp.set("description", "Model node UUID. Defaults to the active model node.");
	properties.set("nodeUUID", nodeProp);
	_tool.set("inputSchema", core::move(inputSchema));
}

bool PlaceVoxelsTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	core::String error;
	scenegraph::SceneGraphNode *node = authoringNode(ctx, args, error);
	if (node == nullptr) {
		return ctx.result(id, error, true);
	}
	if (!args.contains("voxels")) {
		return ctx.result(id, "Missing voxels argument", true);
	}
	const json::Json voxelsArray = args.get("voxels");
	if (!voxelsArray.isArray() || voxelsArray.empty()) {
		return ctx.result(id, "voxels must be a non-empty array", true);
	}
	if (voxelsArray.size() > MaxPlaceVoxels) {
		return ctx.result(id, "At most 50000 voxels per call - use voxedit_draw_shape or voxedit_draw_slices", true);
	}

	PaletteUse use;
	markUsedPaletteSlots(*node->volume(), use);
	core::DynamicArray<PaintCell> cells;
	cells.reserve((size_t)voxelsArray.size());
	bool approximated = false;
	for (int i = 0; i < voxelsArray.size(); ++i) {
		const json::Json voxelData = voxelsArray.get(i);
		PaintCell cell;
		if (voxelData.isArray()) {
			if (voxelData.size() < 4) {
				return ctx.result(id, "Every voxel array must be [x, y, z, color]", true);
			}
			cell.pos = glm::ivec3(voxelData.get(0).intVal(), voxelData.get(1).intVal(), voxelData.get(2).intVal());
			AuthoringColor color;
			if (!resolveAuthoringColor(ctx, *node, voxelData.get(3), use, color, error)) {
				return ctx.result(id, error, true);
			}
			cell.erase = color.erase;
			cell.index = color.index;
			approximated = approximated || color.approximated;
		} else if (voxelData.isObject()) {
			cell.pos = glm::ivec3(voxelData.intVal("x", 0), voxelData.intVal("y", 0), voxelData.intVal("z", 0));
			if (voxelData.contains("color")) {
				AuthoringColor color;
				if (!resolveAuthoringColor(ctx, *node, voxelData.get("color"), use, color, error)) {
					return ctx.result(id, error, true);
				}
				cell.erase = color.erase;
				cell.index = color.index;
				approximated = approximated || color.approximated;
			} else {
				const int colorIndex = voxelData.intVal("idx", 1);
				if (colorIndex < 0 || colorIndex >= node->palette().colorCount()) {
					return ctx.result(id, core::String::format("Palette index %i is outside 0..%i", colorIndex,
															   node->palette().colorCount() - 1),
									  true);
				}
				cell.index = (uint8_t)colorIndex;
			}
		} else {
			return ctx.result(id, "Every voxel must be an object or [x, y, z, color]", true);
		}
		cells.push_back(cell);
	}

	int changed = 0;
	int skipped = 0;
	if (!paintCells(ctx, *node, cells, changed, skipped)) {
		return ctx.result(id, "Volume not found - this is no model node", true);
	}
	if (changed == 0 && skipped > 0) {
		return ctx.result(id, core::String::format("No voxels were placed, %i were outside the volume", skipped), true);
	}
	core::String message = core::String::format("Placed voxels: %i changed, %i outside the volume.", changed, skipped);
	if (approximated) {
		message += " The palette was full, so a closest existing color was used.";
	}
	return ctx.result(id, message, false);
}

} // namespace voxedit
