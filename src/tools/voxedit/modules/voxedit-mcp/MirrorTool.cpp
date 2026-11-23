/**
 * @file
 */

#include "MirrorTool.h"
#include "Authoring.h"

namespace voxedit {

static int mirrorCoord(int coord, int pivot) {
	// Cells pivot and pivot-1 face each other across the plane.
	return 2 * pivot - 1 - coord;
}

MirrorTool::MirrorTool() : Tool("voxedit_mirror") {
	_tool.set("description",
			  "Mirror a model node across a plane. mode \"symmetrize\" (default) copies one side onto the other "
			  "(build one half, get both). mode \"flip\" turns the whole model over. "
			  "pivot is the plane coordinate: cells pivot and pivot-1 face each other. "
			  "When pivot is omitted it is the middle of the volume, so a model built on the high side of the "
			  "center is copied onto the low side. fromNegative copies the low side onto the high side instead. "
			  "Voxels that land outside the volume are dropped. nodeUUID defaults to the active model. Y is up.");

	json::Json inputSchema = objectSchema();
	json::Json required = json::Json::array();
	required.push("axis");
	inputSchema.set("required", required);
	json::Json properties = inputSchema.get("properties");
	json::Json nodeProp = propUUID();
	nodeProp.set("description", "Model node UUID. Defaults to the active model node.");
	properties.set("nodeUUID", nodeProp);
	json::Json axis = propTypeDescription("string", "Mirror axis");
	json::Json axisEnum = json::Json::array();
	axisEnum.push("x");
	axisEnum.push("y");
	axisEnum.push("z");
	axis.set("enum", axisEnum);
	properties.set("axis", axis);
	json::Json mode = propTypeDescription("string", "symmetrize copies one half onto the other. flip turns the model over.");
	json::Json modeEnum = json::Json::array();
	modeEnum.push("symmetrize");
	modeEnum.push("flip");
	mode.set("enum", modeEnum);
	mode.set("default", "symmetrize");
	properties.set("mode", mode);
	properties.set("pivot", propTypeDescription("integer", "Plane coordinate. Default: the middle of the volume on this axis."));
	json::Json fromNegative = propTypeDescription("boolean", "symmetrize: copy the low side onto the high side. Default false.");
	fromNegative.set("default", false);
	properties.set("fromNegative", fromNegative);
	_tool.set("inputSchema", core::move(inputSchema));
}

bool MirrorTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	core::String error;
	scenegraph::SceneGraphNode *node = authoringNode(ctx, args, error);
	if (node == nullptr) {
		return ctx.result(id, error, true);
	}
	math::Axis axis = math::Axis::None;
	if (!parseAxis(args.strVal("axis", ""), axis)) {
		return ctx.result(id, "axis must be x, y or z", true);
	}
	const voxel::RawVolume *volume = node->volume();
	const voxel::Region &region = volume->region();
	int pivot = 0;
	if (args.contains("pivot")) {
		pivot = args.intVal("pivot", 0);
	} else {
		const int lo = axisComponent(region.getLowerCorner(), axis);
		const int hi = axisComponent(region.getUpperCorner(), axis);
		pivot = (lo + hi + 1) / 2;
	}
	const bool flip = args.strVal("mode", "symmetrize").toLower() == "flip";
	const bool fromNegative = args.boolVal("fromNegative", false);

	core::DynamicArray<SolidVoxel> solids;
	if (!collectSolids(*volume, solids, error)) {
		return ctx.result(id, error, true);
	}

	core::DynamicArray<VoxelEdit> edits;
	if (flip) {
		for (const SolidVoxel &solid : solids) {
			VoxelEdit clear;
			clear.pos = solid.pos;
			clear.voxel = voxel::Voxel();
			edits.push_back(clear);
		}
		for (const SolidVoxel &solid : solids) {
			VoxelEdit write;
			write.pos = solid.pos;
			setAxisComponent(write.pos, axis, mirrorCoord(axisComponent(solid.pos, axis), pivot));
			write.voxel = solid.voxel;
			edits.push_back(write);
		}
	} else {
		for (const SolidVoxel &solid : solids) {
			const int coord = axisComponent(solid.pos, axis);
			const bool source = fromNegative ? coord < pivot : coord >= pivot;
			if (source) {
				continue;
			}
			VoxelEdit clear;
			clear.pos = solid.pos;
			clear.voxel = voxel::Voxel();
			edits.push_back(clear);
		}
		for (const SolidVoxel &solid : solids) {
			const int coord = axisComponent(solid.pos, axis);
			const bool source = fromNegative ? coord < pivot : coord >= pivot;
			if (!source) {
				continue;
			}
			VoxelEdit write;
			write.pos = solid.pos;
			setAxisComponent(write.pos, axis, mirrorCoord(coord, pivot));
			write.voxel = solid.voxel;
			edits.push_back(write);
		}
	}

	int changed = 0;
	int skipped = 0;
	if (!applyEdits(ctx, *node, edits, changed, skipped)) {
		return ctx.result(id, "Volume not found - this is no model node", true);
	}
	const char *axisName = axis == math::Axis::X ? "x" : (axis == math::Axis::Y ? "y" : "z");
	if (flip) {
		return ctx.result(id, core::String::format("Flipped across %s = %i. %i voxels changed, %i landed outside the volume.",
												   axisName, pivot, changed, skipped),
						  false);
	}
	return ctx.result(id,
					  core::String::format("Mirrored across %s = %i. %i voxels changed, %i landed outside the volume.",
										   axisName, pivot, changed, skipped),
					  false);
}

} // namespace voxedit
