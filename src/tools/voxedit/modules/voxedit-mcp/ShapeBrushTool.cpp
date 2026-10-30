/**
 * @file
 */

#include "ShapeBrushTool.h"
#include "command/Command.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/ShapeType.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "voxel/RawVolume.h"

namespace voxedit {

ShapeBrushTool::ShapeBrushTool() : BrushTool("voxedit_shape_brush") {
	_tool.set("description", "Create geometric shapes (AABB, Torus, Cylinder, Cone, Dome, Ellipse) in a node's volume");

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
	properties.set("modifierType", propModifierType());

	// Shape type property
	json::Json shapeTypeProp = json::Json::object();
	shapeTypeProp.set("type", "string");
	shapeTypeProp.set("description", "The shape type to create");
	json::Json _enumArr = json::Json::array();
	_enumArr.push("aabb");
	_enumArr.push("torus");
	_enumArr.push("cylinder");
	_enumArr.push("cone");
	_enumArr.push("dome");
	_enumArr.push("ellipse");
	shapeTypeProp.set("enum", _enumArr);
	shapeTypeProp.set("default", "aabb");
	properties.set("shapeType", shapeTypeProp);

	inputSchema.set("properties", core::move(properties));
	_tool.set("inputSchema", core::move(inputSchema));
}

bool ShapeBrushTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
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
	const core::String modifierTypeStr = args.strVal("modifierType", "place").c_str();
	const ModifierType modifierType = parseModifierType(modifierTypeStr);

	const core::String shapeTypeStr = args.strVal("shapeType", "aabb").c_str();

	scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node not found - fetch the scene state first", true);
	}

	voxel::RawVolume *volume = ctx.sceneMgr->volume(node->uuid());
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
	brushContext.gridResolution = glm::ivec3(1);

	// Create the modifier wrapper

	// Get and configure the shape brush
	Modifier &modifier = ctx.sceneMgr->modifier();
	ShapeBrush shapeBrush;

	// Configure the shape brush via command (command is "shape" + type, e.g., "shapeaabb")
	for (int i = 0; i < (int)ShapeType::Max; ++i) {
		if (shapeTypeStr == ShapeTypeCmdStr[i]) {
			shapeBrush.setShapeType((ShapeType)i);
			break;
		}
	}
	shapeBrush.setBoxMode();

	brushContext.evaluatedRegion = voxel::Region(mins, maxs);
	brushContext.cursorPosition = maxs;

	// Execute the brush
	core::Buffer<core::UUID> targets;
	targets.push_back(nodeUUID);
	const BrushApplyResult applied = modifier.executeRequest(shapeBrush, brushContext, targets);
	const bool success = !applied.failed();

	// End the brush operation
	shapeBrush.endBrush(brushContext);

	const voxel::Region &dirtyRegion = applied.dirtyRegion;
	if (dirtyRegion.isValid()) {
		return ctx.result(id, core::String::format("Shape '%s' created successfully", shapeTypeStr.c_str()), false);
	}

	if (success) {
		return ctx.result(id, "Shape brush executed but no voxels were modified", false);
	}

	return ctx.result(id, "Failed to execute shape brush", true);
}

} // namespace voxedit
