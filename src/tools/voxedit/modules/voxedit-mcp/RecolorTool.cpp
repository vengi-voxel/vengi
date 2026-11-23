/**
 * @file
 */

#include "RecolorTool.h"
#include "Authoring.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxedit-util/SceneManager.h"
#include "math/Vec3i.h"

namespace voxedit {

RecolorTool::RecolorTool() : Tool("voxedit_recolor") {
	_tool.set("description",
			  "Replace one color with another on a model node, or on every model node when allNodes is true. "
			  "fromColor and toColor are #RRGGBB or a palette index. toColor -1 erases those voxels. "
			  "nodeUUID defaults to the active model. Y is up.");

	json::Json inputSchema = objectSchema();
	json::Json required = json::Json::array();
	required.push("fromColor");
	required.push("toColor");
	inputSchema.set("required", required);
	json::Json properties = inputSchema.get("properties");
	json::Json nodeProp = propUUID();
	nodeProp.set("description", "Model node UUID. Defaults to the active model node. Ignored when allNodes is true.");
	properties.set("nodeUUID", nodeProp);
	json::Json colorOneOf = json::Json::array();
	colorOneOf.push(propTypeDescription("string", "#RRGGBB"));
	colorOneOf.push(propTypeDescription("integer", "Palette index. -1 erases."));
	json::Json fromColor = json::Json::object();
	fromColor.set("description", "#RRGGBB or palette index of the voxels to change");
	fromColor.set("oneOf", colorOneOf);
	json::Json toColor = json::Json::object();
	toColor.set("description", "#RRGGBB or palette index to write. -1 erases.");
	toColor.set("oneOf", colorOneOf);
	properties.set("fromColor", fromColor);
	properties.set("toColor", toColor);
	json::Json allNodes = propTypeDescription("boolean", "Recolor every model node. Default false.");
	allNodes.set("default", false);
	properties.set("allNodes", allNodes);
	_tool.set("inputSchema", core::move(inputSchema));
}

static int recolorNode(ToolContext &ctx, scenegraph::SceneGraphNode &node, const AuthoringColor &from,
					   const AuthoringColor &to) {
	const voxel::RawVolume *volume = node.volume();
	if (volume == nullptr || from.erase) {
		return 0;
	}
	core::DynamicArray<PaintCell> cells;
	voxelutil::visitVolume(*volume, [&cells, &from](int x, int y, int z, const voxel::Voxel &voxel) {
		if (voxel.getColor() != from.index) {
			return;
		}
		PaintCell cell;
		cell.pos = math::Vec3i(x, y, z);
		cell.erase = false;
		cell.index = 0;
		cells.push_back(cell);
	}, voxelutil::VisitSolid());
	for (PaintCell &cell : cells) {
		cell.erase = to.erase;
		cell.index = to.index;
	}
	int changed = 0;
	int skipped = 0;
	paintCells(ctx, node, cells, changed, skipped);
	return changed;
}

bool RecolorTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	if (!args.contains("fromColor") || !args.contains("toColor")) {
		return ctx.result(id, "fromColor and toColor are required", true);
	}
	const bool allNodes = args.boolVal("allNodes", false);
	core::String error;
	int changed = 0;
	bool approximated = false;
	if (allNodes) {
		const scenegraph::SceneGraph &graph = ctx.sceneMgr->sceneGraph();
		for (auto iter = graph.beginModel(); iter != graph.end(); ++iter) {
			scenegraph::SceneGraphNode &node = *iter;
			if (node.volume() == nullptr) {
				continue;
			}
			PaletteUse use;
			if (node.volume() != nullptr) {
				markUsedPaletteSlots(*node.volume(), use);
			}
			AuthoringColor from;
			AuthoringColor to;
			if (!resolveAuthoringColor(ctx, node, args.get("fromColor"), use, from, error) ||
				!resolveAuthoringColor(ctx, node, args.get("toColor"), use, to, error)) {
				return ctx.result(id, error, true);
			}
			if (from.erase) {
				return ctx.result(id, "fromColor must be an existing color, not 0", true);
			}
			approximated = approximated || from.approximated || to.approximated;
			changed += recolorNode(ctx, node, from, to);
		}
	} else {
		scenegraph::SceneGraphNode *node = authoringNode(ctx, args, error);
		if (node == nullptr) {
			return ctx.result(id, error, true);
		}
		PaletteUse use;
		markUsedPaletteSlots(*node->volume(), use);
		AuthoringColor from;
		AuthoringColor to;
		if (!resolveAuthoringColor(ctx, *node, args.get("fromColor"), use, from, error) ||
			!resolveAuthoringColor(ctx, *node, args.get("toColor"), use, to, error)) {
			return ctx.result(id, error, true);
		}
		if (from.erase) {
			return ctx.result(id, "fromColor must be an existing color, not 0", true);
		}
		approximated = from.approximated || to.approximated;
		changed = recolorNode(ctx, *node, from, to);
	}
	core::String message = core::String::format("Recolored %i voxels.", changed);
	if (approximated) {
		message += " The palette was full, so a closest existing color was used.";
	}
	return ctx.result(id, message, false);
}

} // namespace voxedit
