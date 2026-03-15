/**
 * @file
 */

#include "TransformBrushTool.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/TransformBrush.h"
#include "voxel/RawVolume.h"

namespace voxedit {

static TransformMode parseTransformMode(const core::String &mode) {
	if (mode == "shear") {
		return TransformMode::Shear;
	}
	if (mode == "scale") {
		return TransformMode::Scale;
	}
	if (mode == "rotate") {
		return TransformMode::Rotate;
	}
	return TransformMode::Move;
}

TransformBrushTool::TransformBrushTool() : BrushTool("voxedit_transform_brush") {
	_tool.set("description",
			  "Transform selected voxels with various modes (Move, Shear, Scale, Rotate). "
			  "Requires a selection first (use voxedit_select_brush to select voxels before transforming).");

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	json::Json requiredArr = json::Json::array();
	requiredArr.push("nodeUUID");
	requiredArr.push("transformMode");
	inputSchema.set("required", requiredArr);

	json::Json properties = json::Json::object();
	properties.set("nodeUUID", propUUID());

	// Transform mode property
	json::Json transformModeProp = json::Json::object();
	transformModeProp.set("type", "string");
	transformModeProp.set("description",
						  "The transform mode: 'move' (translate voxels), 'shear' (shear offset), "
						  "'scale' (resize), 'rotate' (rotate around center)");
	json::Json enumArr = json::Json::array();
	enumArr.push("move");
	enumArr.push("shear");
	enumArr.push("scale");
	enumArr.push("rotate");
	transformModeProp.set("enum", enumArr);
	transformModeProp.set("default", "move");
	properties.set("transformMode", transformModeProp);

	// Move offset
	json::Json moveOffsetProp = json::Json::object();
	moveOffsetProp.set("type", "object");
	moveOffsetProp.set("description", "Move offset in voxels (for 'move' mode). Each axis range: -128 to 128.");
	json::Json moveProps = json::Json::object();
	moveProps.set("x", propTypeDescription("integer", "X offset"));
	moveProps.set("y", propTypeDescription("integer", "Y offset"));
	moveProps.set("z", propTypeDescription("integer", "Z offset"));
	moveOffsetProp.set("properties", core::move(moveProps));
	properties.set("moveOffset", core::move(moveOffsetProp));

	// Shear offset
	json::Json shearOffsetProp = json::Json::object();
	shearOffsetProp.set("type", "object");
	shearOffsetProp.set("description", "Shear offset in voxels (for 'shear' mode). Each axis range: -128 to 128.");
	json::Json shearProps = json::Json::object();
	shearProps.set("x", propTypeDescription("integer", "X shear"));
	shearProps.set("y", propTypeDescription("integer", "Y shear"));
	shearProps.set("z", propTypeDescription("integer", "Z shear"));
	shearOffsetProp.set("properties", core::move(shearProps));
	properties.set("shearOffset", core::move(shearOffsetProp));

	// Scale
	json::Json scaleProp = json::Json::object();
	scaleProp.set("type", "object");
	scaleProp.set("description", "Scale factors (for 'scale' mode). 1.0 = no change.");
	json::Json scaleProps = json::Json::object();
	scaleProps.set("x", propTypeDescription("number", "X scale factor"));
	scaleProps.set("y", propTypeDescription("number", "Y scale factor"));
	scaleProps.set("z", propTypeDescription("number", "Z scale factor"));
	scaleProp.set("properties", core::move(scaleProps));
	properties.set("scale", core::move(scaleProp));

	// Rotation degrees
	json::Json rotationProp = json::Json::object();
	rotationProp.set("type", "object");
	rotationProp.set("description", "Rotation in degrees around each axis (for 'rotate' mode).");
	json::Json rotProps = json::Json::object();
	rotProps.set("x", propTypeDescription("number", "Rotation around X axis in degrees"));
	rotProps.set("y", propTypeDescription("number", "Rotation around Y axis in degrees"));
	rotProps.set("z", propTypeDescription("number", "Rotation around Z axis in degrees"));
	rotationProp.set("properties", core::move(rotProps));
	properties.set("rotation", core::move(rotationProp));

	inputSchema.set("properties", core::move(properties));
	_tool.set("inputSchema", core::move(inputSchema));
}

bool TransformBrushTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}

	const core::String transformModeStr = args.strVal("transformMode", "move").c_str();
	const TransformMode transformMode = parseTransformMode(transformModeStr);

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
	brushContext.modifierType = ModifierType::Override;
	brushContext.cursorPosition = volume->region().getCenter();
	brushContext.referencePos = brushContext.cursorPosition;
	brushContext.cursorFace = voxel::FaceNames::PositiveY;
	brushContext.targetVolumeRegion = volume->region();
	brushContext.gridResolution = 1;

	// Create the modifier wrapper
	ModifierVolumeWrapper wrapper(*node, ModifierType::Override);

	// Get and configure the transform brush
	Modifier &modifier = ctx.sceneMgr->modifier();
	TransformBrush &transformBrush = modifier.transformBrush();

	// Save previous state
	const BrushType prevBrushType = modifier.brushType();
	const TransformMode prevTransformMode = transformBrush.transformMode();
	const glm::ivec3 prevMoveOffset = transformBrush.moveOffset();
	const glm::ivec3 prevShearOffset = transformBrush.shearOffset();
	const glm::vec3 prevScale = transformBrush.scale();
	const glm::vec3 prevRotation = transformBrush.rotationDegrees();

	// Configure the transform brush
	modifier.setBrushType(BrushType::Transform);
	transformBrush.setTransformMode(transformMode);

	switch (transformMode) {
	case TransformMode::Move:
		if (args.contains("moveOffset")) {
			const json::Json &offset = args.get("moveOffset");
			transformBrush.setMoveOffset(
				glm::ivec3(offset.intVal("x", 0), offset.intVal("y", 0), offset.intVal("z", 0)));
		}
		break;
	case TransformMode::Shear:
		if (args.contains("shearOffset")) {
			const json::Json &offset = args.get("shearOffset");
			transformBrush.setShearOffset(
				glm::ivec3(offset.intVal("x", 0), offset.intVal("y", 0), offset.intVal("z", 0)));
		}
		break;
	case TransformMode::Scale:
		if (args.contains("scale")) {
			const json::Json &s = args.get("scale");
			transformBrush.setScale(
				glm::vec3(s.floatVal("x", 1.0f), s.floatVal("y", 1.0f), s.floatVal("z", 1.0f)));
		}
		break;
	case TransformMode::Rotate:
		if (args.contains("rotation")) {
			const json::Json &r = args.get("rotation");
			transformBrush.setRotationDegrees(
				glm::vec3(r.floatVal("x", 0.0f), r.floatVal("y", 0.0f), r.floatVal("z", 0.0f)));
		}
		break;
	default:
		break;
	}

	// Begin and execute the brush
	transformBrush.beginBrush(brushContext);
	transformBrush.preExecute(brushContext, volume);
	const bool success = transformBrush.execute(ctx.sceneMgr->sceneGraph(), wrapper, brushContext);

	// End the brush operation
	transformBrush.endBrush(brushContext);

	// Restore previous state
	transformBrush.setTransformMode(prevTransformMode);
	transformBrush.setMoveOffset(prevMoveOffset);
	transformBrush.setShearOffset(prevShearOffset);
	transformBrush.setScale(prevScale);
	transformBrush.setRotationDegrees(prevRotation);
	modifier.setBrushType(prevBrushType);

	const voxel::Region &dirtyRegion = wrapper.dirtyRegion();
	if (dirtyRegion.isValid()) {
		ctx.sceneMgr->modified(node->id(), dirtyRegion);
		return ctx.result(id,
						  core::String::format("Transform '%s' executed successfully", transformModeStr.c_str()),
						  false);
	}

	if (success) {
		return ctx.result(id,
						  "Transform brush executed but no voxels were modified - ensure voxels are selected first",
						  false);
	}

	return ctx.result(
		id, "Failed to execute transform brush - ensure voxels are selected first (use voxedit_select_brush)", true);
}

} // namespace voxedit
