/**
 * @file
 */

#include "ExtrudeBrushTool.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/ExtrudeBrush.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"

namespace voxedit {

ExtrudeBrushTool::ExtrudeBrushTool() : BrushTool("voxedit_extrude_brush") {
	_tool.set("description",
			  "Extrude or carve selected voxels along a face direction. Positive depth extrudes outward, "
			  "negative depth carves inward. Requires a selection first (use voxedit_select_brush to select "
			  "voxels before extruding).");

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	json::Json requiredArr = json::Json::array();
	requiredArr.push("nodeUUID");
	requiredArr.push("face");
	requiredArr.push("depth");
	inputSchema.set("required", requiredArr);

	json::Json properties = json::Json::object();
	properties.set("nodeUUID", propUUID());

	// Face direction property
	json::Json faceProp = json::Json::object();
	faceProp.set("type", "string");
	faceProp.set("description",
				 "The face direction for extrusion: 'positivex', 'negativex', 'positivey', 'negativey', "
				 "'positivez', 'negativez' (or aliases like 'up', 'down', 'left', 'right', 'east', 'west', etc.)");
	properties.set("face", faceProp);

	// Depth property
	json::Json depthProp = json::Json::object();
	depthProp.set("type", "integer");
	depthProp.set("description", "Extrusion depth in voxels. Positive = extrude outward, negative = carve inward.");
	depthProp.set("default", 1);
	properties.set("depth", depthProp);

	// Modifier type (place or erase)
	json::Json modTypeProp = json::Json::object();
	modTypeProp.set("type", "string");
	modTypeProp.set("description",
					"The modifier type: 'place' (extrude/add voxels outward), 'erase' (remove outermost layer)");
	json::Json enumArr = json::Json::array();
	enumArr.push("place");
	enumArr.push("erase");
	modTypeProp.set("enum", enumArr);
	modTypeProp.set("default", "place");
	properties.set("modifierType", modTypeProp);

	// Fill walls option
	json::Json fillWallsProp = json::Json::object();
	fillWallsProp.set("type", "boolean");
	fillWallsProp.set("description", "Whether to fill side walls when extruding (default: true)");
	fillWallsProp.set("default", true);
	properties.set("fillWalls", fillWallsProp);

	// Lateral offset U
	json::Json offsetUProp = json::Json::object();
	offsetUProp.set("type", "integer");
	offsetUProp.set("description",
					"Lateral offset along the first perpendicular axis (shifts extrusion sideways)");
	offsetUProp.set("default", 0);
	properties.set("offsetU", offsetUProp);

	// Lateral offset V
	json::Json offsetVProp = json::Json::object();
	offsetVProp.set("type", "integer");
	offsetVProp.set("description",
					"Lateral offset along the second perpendicular axis (shifts extrusion sideways)");
	offsetVProp.set("default", 0);
	properties.set("offsetV", offsetVProp);

	inputSchema.set("properties", core::move(properties));
	_tool.set("inputSchema", core::move(inputSchema));
}

bool ExtrudeBrushTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}

	const core::String faceStr = args.strVal("face", "").c_str();
	if (faceStr.empty()) {
		return ctx.result(id, "Missing required parameter 'face'", true);
	}
	const voxel::FaceNames face = voxel::toFaceNames(faceStr);
	if (face == voxel::FaceNames::Max) {
		return ctx.result(id, core::String::format("Invalid face direction: '%s'", faceStr.c_str()), true);
	}

	const int depth = args.intVal("depth", 1);
	const core::String modifierTypeStr = args.strVal("modifierType", "place").c_str();
	const ModifierType modifierType = parseModifierType(modifierTypeStr);
	const bool fillWalls = args.boolVal("fillWalls", true);
	const int offsetU = args.intVal("offsetU", 0);
	const int offsetV = args.intVal("offsetV", 0);

	scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node not found - fetch the scene state first", true);
	}

	voxel::RawVolume *volume = ctx.sceneMgr->volume(node->id());
	if (volume == nullptr) {
		return ctx.result(id, "Volume not found - this is no model node", true);
	}

	// Create the brush context
	BrushContext brushContext;
	brushContext.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	brushContext.modifierType = modifierType;
	brushContext.cursorPosition = volume->region().getCenter();
	brushContext.referencePos = brushContext.cursorPosition;
	brushContext.cursorFace = face;
	brushContext.targetVolumeRegion = volume->region();
	brushContext.gridResolution = 1;

	// Create the modifier wrapper
	ModifierVolumeWrapper wrapper(*node, modifierType);

	// Get and configure the extrude brush
	Modifier &modifier = ctx.sceneMgr->modifier();
	ExtrudeBrush &extrudeBrush = modifier.extrudeBrush();

	// Save previous state
	const BrushType prevBrushType = modifier.brushType();
	const int prevDepth = extrudeBrush.depth();
	const int prevOffsetU = extrudeBrush.offsetU();
	const int prevOffsetV = extrudeBrush.offsetV();
	const bool prevFillWalls = extrudeBrush.fillWalls();

	// Configure the extrude brush
	modifier.setBrushType(BrushType::Extrude);
	extrudeBrush.setDepth(depth);
	extrudeBrush.setOffsetU(offsetU);
	extrudeBrush.setOffsetV(offsetV);
	extrudeBrush.setFillWalls(fillWalls);

	// Begin and execute the brush
	extrudeBrush.beginBrush(brushContext);
	extrudeBrush.preExecute(brushContext, volume);
	const bool success = extrudeBrush.execute(ctx.sceneMgr->sceneGraph(), wrapper, brushContext);

	// End the brush operation
	extrudeBrush.endBrush(brushContext);

	// Restore previous state
	extrudeBrush.setDepth(prevDepth);
	extrudeBrush.setOffsetU(prevOffsetU);
	extrudeBrush.setOffsetV(prevOffsetV);
	extrudeBrush.setFillWalls(prevFillWalls);
	modifier.setBrushType(prevBrushType);

	const voxel::Region &dirtyRegion = wrapper.dirtyRegion();
	if (dirtyRegion.isValid()) {
		ctx.sceneMgr->modified(node->id(), dirtyRegion);
		return ctx.result(id,
						  core::String::format("Extrude executed successfully (face=%s, depth=%d)",
											   faceStr.c_str(), depth),
						  false);
	}

	if (success) {
		return ctx.result(id,
						  "Extrude brush executed but no voxels were modified - ensure voxels are selected first",
						  false);
	}

	return ctx.result(
		id, "Failed to execute extrude brush - ensure voxels are selected first (use voxedit_select_brush)", true);
}

} // namespace voxedit
