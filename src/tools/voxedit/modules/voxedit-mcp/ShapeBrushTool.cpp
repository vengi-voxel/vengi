/**
 * @file
 */

#include "ShapeBrushTool.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/ShapeType.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "voxel/RawVolume.h"

namespace voxedit {

ShapeBrushTool::ShapeBrushTool() : BrushTool("voxedit_shape_brush") {
	_tool["description"] = "Create geometric shapes (AABB, Torus, Cylinder, Cone, Dome, Ellipse) in a node's volume";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"nodeUUID", "region"});

	nlohmann::json properties;
	properties["nodeUUID"] = propUUID();
	properties["region"] = propRegion();
	properties["colorIndex"] = propColorIndex();
	properties["modifierType"] = propModifierType();

	// Shape type property
	nlohmann::json shapeTypeProp;
	shapeTypeProp["type"] = "string";
	shapeTypeProp["description"] = "The shape type to create";
	shapeTypeProp["enum"] = nlohmann::json::array({"aabb", "torus", "cylinder", "cone", "dome", "ellipse"});
	shapeTypeProp["default"] = "aabb";
	properties["shapeType"] = shapeTypeProp;

	inputSchema["properties"] = core::move(properties);
	_tool["inputSchema"] = core::move(inputSchema);
}

bool ShapeBrushTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
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
	const core::String modifierTypeStr = args.value("modifierType", "place").c_str();
	const ModifierType modifierType = parseModifierType(modifierTypeStr);

	const core::String shapeTypeStr = args.value("shapeType", "aabb").c_str();

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
	brushContext.modifierType = modifierType;
	brushContext.cursorPosition = mins;
	brushContext.referencePos = mins;
	brushContext.cursorFace = voxel::FaceNames::PositiveY;
	brushContext.targetVolumeRegion = volume->region();
	brushContext.gridResolution = 1;

	// Create the modifier wrapper
	ModifierVolumeWrapper wrapper(*node, modifierType);

	// Get and configure the shape brush
	Modifier &modifier = ctx.sceneMgr->modifier();
	ShapeBrush &shapeBrush = modifier.shapeBrush();

	// Save previous state
	const ShapeType prevShapeType = shapeBrush.shapeType();
	const BrushType prevBrushType = modifier.brushType();

	// Configure the shape brush via command (command is "shape" + type, e.g., "shapeaabb")
	command::Command::execute(core::String::format("shape%s", shapeTypeStr.c_str()));
	modifier.setBrushType(BrushType::Shape);
	shapeBrush.setAABBMode();

	// Set up the AABB region
	brushContext.cursorPosition = mins;
	shapeBrush.beginBrush(brushContext);
	brushContext.cursorPosition = maxs;
	shapeBrush.step(brushContext);

	// Execute the brush
	shapeBrush.preExecute(brushContext, volume);
	const bool success = shapeBrush.execute(ctx.sceneMgr->sceneGraph(), wrapper, brushContext);

	// End the brush operation
	shapeBrush.endBrush(brushContext);

	// Restore previous state
	command::Command::execute(
		core::String::format("shape%s", core::String::lower(ShapeTypeStr[prevShapeType]).c_str()));
	modifier.setBrushType(prevBrushType);

	const voxel::Region &dirtyRegion = wrapper.dirtyRegion();
	if (dirtyRegion.isValid()) {
		ctx.sceneMgr->modified(node->id(), dirtyRegion);
		return ctx.result(id, core::String::format("Shape '%s' created successfully", shapeTypeStr.c_str()), false);
	}

	if (success) {
		return ctx.result(id, "Shape brush executed but no voxels were modified", false);
	}

	return ctx.result(id, "Failed to execute shape brush", true);
}

} // namespace voxedit
