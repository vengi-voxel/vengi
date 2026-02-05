/**
 * @file
 */

#include "PlaneBrushTool.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/PlaneBrush.h"
#include "voxel/RawVolume.h"

namespace voxedit {

// TODO: MCP:  use toFaceNames
static voxel::FaceNames parseFace(const core::String &face) {
	if (face == "positivex" || face == "+x") {
		return voxel::FaceNames::PositiveX;
	}
	if (face == "negativex" || face == "-x") {
		return voxel::FaceNames::NegativeX;
	}
	if (face == "positivey" || face == "+y") {
		return voxel::FaceNames::PositiveY;
	}
	if (face == "negativey" || face == "-y") {
		return voxel::FaceNames::NegativeY;
	}
	if (face == "positivez" || face == "+z") {
		return voxel::FaceNames::PositiveZ;
	}
	if (face == "negativez" || face == "-z") {
		return voxel::FaceNames::NegativeZ;
	}
	return voxel::FaceNames::PositiveY;
}

PlaneBrushTool::PlaneBrushTool() : BrushTool("voxedit_plane_brush") {
	_tool["description"] =
		"Extrude or fill a plane of voxels along a face direction. Fills all connected voxels on the hit surface.";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"nodeUUID", "position"});

	nlohmann::json properties;
	properties["nodeUUID"] = propUUID();
	properties["position"] = propPosition("The position to start the plane extrusion from");
	properties["colorIndex"] = propColorIndex();
	properties["modifierType"] = propModifierType();

	// Face direction property
	nlohmann::json faceProp;
	faceProp["type"] = "string";
	faceProp["description"] = "The face direction for the plane extrusion";
	faceProp["enum"] = nlohmann::json::array({"+x", "-x", "+y", "-y", "+z", "-z", "positivex", "negativex", "positivey",
											  "negativey", "positivez", "negativez"});
	faceProp["default"] = "+y";
	properties["face"] = faceProp;

	// Thickness property
	nlohmann::json thicknessProp;
	thicknessProp["type"] = "integer";
	thicknessProp["description"] = "The thickness of the extrusion in voxels";
	thicknessProp["default"] = 1;
	thicknessProp["minimum"] = 1;
	properties["thickness"] = thicknessProp;

	inputSchema["properties"] = core::move(properties);
	_tool["inputSchema"] = core::move(inputSchema);
}

bool PlaneBrushTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}

	if (!args.contains("position")) {
		return ctx.result(id, "Missing position argument", true);
	}
	const nlohmann::json &posJson = args["position"];

	const glm::ivec3 position(posJson.value("x", 0), posJson.value("y", 0), posJson.value("z", 0));
	const int colorIndex = args.value("colorIndex", 0);
	const core::String modifierTypeStr = args.value("modifierType", "place").c_str();
	const ModifierType modifierType = parseModifierType(modifierTypeStr);
	const core::String faceStr = args.value("face", "+y").c_str();
	const voxel::FaceNames face = parseFace(faceStr);
	const int thickness = args.value("thickness", 1);

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

	// Get the hit voxel at the position
	const voxel::Voxel hitVoxel = volume->voxel(position);

	// Create the brush context
	BrushContext brushContext;
	brushContext.cursorVoxel = cursorVoxel;
	brushContext.hitCursorVoxel = hitVoxel;
	brushContext.modifierType = modifierType;
	brushContext.cursorPosition = position;
	brushContext.referencePos = position;
	brushContext.cursorFace = face;
	brushContext.targetVolumeRegion = volume->region();
	brushContext.gridResolution = thickness;

	// Create the modifier wrapper
	ModifierVolumeWrapper wrapper(*node, modifierType);

	// Get and configure the plane brush
	Modifier &modifier = ctx.sceneMgr->modifier();
	PlaneBrush &planeBrush = modifier.planeBrush();

	// Save previous state
	const BrushType prevBrushType = modifier.brushType();

	// Configure the plane brush
	modifier.setBrushType(BrushType::Plane);
	planeBrush.setAABBMode();

	// Begin and execute the brush
	planeBrush.beginBrush(brushContext);
	planeBrush.preExecute(brushContext, volume);
	const bool success = planeBrush.execute(ctx.sceneMgr->sceneGraph(), wrapper, brushContext);

	// End the brush operation
	planeBrush.endBrush(brushContext);

	// Restore previous state
	modifier.setBrushType(prevBrushType);

	const voxel::Region &dirtyRegion = wrapper.dirtyRegion();
	if (dirtyRegion.isValid()) {
		ctx.sceneMgr->modified(node->id(), dirtyRegion);
		return ctx.result(id, "Plane extrusion executed successfully", false);
	}

	if (success) {
		return ctx.result(id, "Plane brush executed but no voxels were modified", false);
	}

	return ctx.result(id, "Failed to execute plane brush", true);
}

} // namespace voxedit
