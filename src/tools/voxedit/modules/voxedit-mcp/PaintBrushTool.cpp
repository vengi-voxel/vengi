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
	_tool.set("description", "Paint/recolor existing voxels in a region with various modes (Replace, Brighten, Darken, Random, Variation)");

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	json::Json _requiredArr = json::Json::array();
	_requiredArr.push("nodeUUID");
	_requiredArr.push("region");
	inputSchema.set("required", _requiredArr);

	json::Json properties = json::Json::object();
	properties.set("nodeUUID", propUUID());
	properties.set("region", propRegion());
	properties.set("colorIndex", propColorIndex());

	// Paint mode property
	json::Json paintModeProp = json::Json::object();
	paintModeProp.set("type", "string");
	paintModeProp.set("description",
		"The paint mode: 'replace' (change to new color), 'brighten' (make lighter), 'darken' (make darker), "
		"'random' (random palette colors), 'variation' (random brightness variation)");
	json::Json _enumArr = json::Json::array();
	_enumArr.push("replace");
	_enumArr.push("brighten");
	_enumArr.push("darken");
	_enumArr.push("random");
	_enumArr.push("variation");
	paintModeProp.set("enum", _enumArr);
	paintModeProp.set("default", "replace");
	properties.set("paintMode", paintModeProp);

	// Factor property for brighten/darken
	json::Json factorProp = json::Json::object();
	factorProp.set("type", "number");
	factorProp.set("description", "Brightness factor for brighten/darken modes (1.0 = no change, >1.0 = brighter, <1.0 = darker)");
	factorProp.set("default", 1.2);
	factorProp.set("minimum", 0.1);
	factorProp.set("maximum", 3.0);
	properties.set("factor", factorProp);

	inputSchema.set("properties", core::move(properties));
	_tool.set("inputSchema", core::move(inputSchema));
}

bool PaintBrushTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}

	if (!args.contains("region")) {
		return ctx.result(id, "Missing region argument", true);
	}
	json::Json region = args.get("region");

	const glm::ivec3 mins(region.intVal("minX", 0), region.intVal("minY", 0), region.intVal("minZ", 0));
	const glm::ivec3 maxs(region.intVal("maxX", 0), region.intVal("maxY", 0), region.intVal("maxZ", 0));

	const int colorIndex = args.intVal("colorIndex", 0);
	const core::String paintModeStr = args.strVal("paintMode", "replace").c_str();
	const PaintBrush::PaintMode paintMode = parsePaintMode(paintModeStr);
	const float factor = args.floatVal("factor", 1.2f);

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
