/**
 * @file
 */

#include "PaintBrushTool.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/PaintBrush.h"
#include "voxel/RawVolume.h"

namespace voxedit {

static PaintBrush::PaintMode parsePaintMode(const core::String &mode) {
	if (mode == "brighten") {
		return PaintBrush::PaintMode::Brighten;
	}
	if (mode == "darken") {
		return PaintBrush::PaintMode::Darken;
	}
	if (mode == "random") {
		return PaintBrush::PaintMode::Random;
	}
	if (mode == "variation") {
		return PaintBrush::PaintMode::Variation;
	}
	return PaintBrush::PaintMode::Replace;
}

PaintBrushTool::PaintBrushTool() : BrushTool("voxedit_paint_brush") {
	_tool["description"] =
		"Paint/recolor existing voxels in a region with various modes (Replace, Brighten, Darken, Random, Variation)";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"nodeUUID", "region"});

	nlohmann::json properties;
	properties["nodeUUID"] = propUUID();
	properties["region"] = propRegion();
	properties["colorIndex"] = propColorIndex();

	// Paint mode property
	nlohmann::json paintModeProp;
	paintModeProp["type"] = "string";
	paintModeProp["description"] =
		"The paint mode: 'replace' (change to new color), 'brighten' (make lighter), 'darken' (make darker), "
		"'random' (random palette colors), 'variation' (random brightness variation)";
	paintModeProp["enum"] = nlohmann::json::array({"replace", "brighten", "darken", "random", "variation"});
	paintModeProp["default"] = "replace";
	properties["paintMode"] = paintModeProp;

	// Factor property for brighten/darken
	nlohmann::json factorProp;
	factorProp["type"] = "number";
	factorProp["description"] = "Brightness factor for brighten/darken modes (1.0 = no change, >1.0 = brighter, <1.0 = darker)";
	factorProp["default"] = 1.2;
	factorProp["minimum"] = 0.1;
	factorProp["maximum"] = 3.0;
	properties["factor"] = factorProp;

	inputSchema["properties"] = core::move(properties);
	_tool["inputSchema"] = core::move(inputSchema);
}

bool PaintBrushTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}

	if (!args.contains("region")) {
		return ctx.result(id, "Missing region argument", true);
	}
	const nlohmann::json &region = args["region"];

	const glm::ivec3 mins(region.value("minX", 0), region.value("minY", 0), region.value("minZ", 0));
	const glm::ivec3 maxs(region.value("maxX", 0), region.value("maxY", 0), region.value("maxZ", 0));

	const int colorIndex = args.value("colorIndex", 0);
	const core::String paintModeStr = args.value("paintMode", "replace").c_str();
	const PaintBrush::PaintMode paintMode = parsePaintMode(paintModeStr);
	const float factor = args.value("factor", 1.2f);

	scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node not found - fetch the scene state first", true);
	}

	voxel::RawVolume *volume = ctx.sceneMgr->volume(node->id());
	if (volume == nullptr) {
		return ctx.result(id, "Volume not found - this is no model node", true);
	}

	// Create a voxel with the specified color
	const voxel::Voxel cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, colorIndex);

	// Create the brush context
	BrushContext brushContext;
	brushContext.cursorVoxel = cursorVoxel;
	brushContext.modifierType = ModifierType::Paint;
	brushContext.cursorPosition = mins;
	brushContext.referencePos = mins;
	brushContext.cursorFace = voxel::FaceNames::PositiveY;
	brushContext.targetVolumeRegion = volume->region();
	brushContext.gridResolution = 1;

	// Create the modifier wrapper
	ModifierVolumeWrapper wrapper(*node, ModifierType::Paint);

	// Get and configure the paint brush
	Modifier &modifier = ctx.sceneMgr->modifier();
	PaintBrush &paintBrush = modifier.paintBrush();

	// Save previous state
	const BrushType prevBrushType = modifier.brushType();
	const PaintBrush::PaintMode prevPaintMode = paintBrush.paintMode();
	const float prevFactor = paintBrush.factor();

	// Configure the paint brush
	modifier.setBrushType(BrushType::Paint);
	paintBrush.setPaintMode(paintMode);
	paintBrush.setFactor(factor);
	paintBrush.setAABBMode();

	// Set up the AABB region
	brushContext.cursorPosition = mins;
	paintBrush.beginBrush(brushContext);
	brushContext.cursorPosition = maxs;
	paintBrush.step(brushContext);

	// Execute the brush
	paintBrush.preExecute(brushContext, volume);
	const bool success = paintBrush.execute(ctx.sceneMgr->sceneGraph(), wrapper, brushContext);

	// End the brush operation
	paintBrush.endBrush(brushContext);

	// Restore previous state
	paintBrush.setPaintMode(prevPaintMode);
	paintBrush.setFactor(prevFactor);
	modifier.setBrushType(prevBrushType);

	const voxel::Region &dirtyRegion = wrapper.dirtyRegion();
	if (dirtyRegion.isValid()) {
		ctx.sceneMgr->modified(node->id(), dirtyRegion);
		return ctx.result(id, core::String::format("Paint brush '%s' executed successfully", paintModeStr.c_str()),
						  false);
	}

	if (success) {
		return ctx.result(id, "Paint brush executed but no voxels were modified", false);
	}

	return ctx.result(id, "Failed to execute paint brush", true);
}

} // namespace voxedit
