/**
 * @file
 */

#include "LineBrushTool.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/LineBrush.h"
#include "voxel/RawVolume.h"

namespace voxedit {

LineBrushTool::LineBrushTool() : BrushTool("voxedit_line_brush") {
	_tool.set("description", "Draw a straight line of voxels between two positions");

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	json::Json _requiredArr = json::Json::array();
	_requiredArr.push("nodeUUID");
	_requiredArr.push("start");
	_requiredArr.push("end");
	inputSchema.set("required", _requiredArr);

	json::Json properties = json::Json::object();
	properties.set("nodeUUID", propUUID());
	properties.set("start", propPosition("The starting position of the line"));
	properties.set("end", propPosition("The ending position of the line"));
	properties.set("colorIndex", propColorIndex());
	properties.set("modifierType", propModifierType());

	// Continuous mode property
	json::Json continuousProp = json::Json::object();
	continuousProp.set("type", "boolean");
	continuousProp.set("description", "If true, the end position becomes the start of the next line segment");
	continuousProp.set("default", false);
	properties.set("continuous", continuousProp);

	inputSchema.set("properties", core::move(properties));
	_tool.set("inputSchema", core::move(inputSchema));
}

bool LineBrushTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}

	if (!args.contains("start")) {
		return ctx.result(id, "Missing start position argument", true);
	}
	if (!args.contains("end")) {
		return ctx.result(id, "Missing end position argument", true);
	}

	json::Json startPos = args.get("start");
	json::Json endPos = args.get("end");

	const glm::ivec3 start(startPos.intVal("x", 0), startPos.intVal("y", 0), startPos.intVal("z", 0));
	const glm::ivec3 end(endPos.intVal("x", 0), endPos.intVal("y", 0), endPos.intVal("z", 0));

	const int colorIndex = args.intVal("colorIndex", 0);
	const core::String modifierTypeStr = args.strVal("modifierType", "place").c_str();
	const ModifierType modifierType = parseModifierType(modifierTypeStr);
	const bool continuous = args.boolVal("continuous", false);

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
	brushContext.referencePos = start;
	brushContext.cursorPosition = end;
	brushContext.cursorFace = voxel::FaceNames::PositiveY;
	brushContext.targetVolumeRegion = volume->region();
	brushContext.gridResolution = 1;

	// Create the modifier wrapper
	ModifierVolumeWrapper wrapper(*node, modifierType);

	// Get and configure the line brush
	Modifier &modifier = ctx.sceneMgr->modifier();
	LineBrush &lineBrush = modifier.lineBrush();

	// Save previous state
	const BrushType prevBrushType = modifier.brushType();
	const bool prevContinuous = lineBrush.continuous();

	// Configure the line brush
	modifier.setBrushType(BrushType::Line);
	lineBrush.setContinuous(continuous);

	// Execute the brush
	lineBrush.preExecute(brushContext, volume);
	const bool success = lineBrush.execute(ctx.sceneMgr->sceneGraph(), wrapper, brushContext);

	// End the brush operation
	lineBrush.endBrush(brushContext);

	// Restore previous state
	lineBrush.setContinuous(prevContinuous);
	modifier.setBrushType(prevBrushType);

	const voxel::Region &dirtyRegion = wrapper.dirtyRegion();
	if (dirtyRegion.isValid()) {
		ctx.sceneMgr->modified(node->id(), dirtyRegion);
		return ctx.result(id, "Line drawn successfully", false);
	}

	if (success) {
		return ctx.result(id, "Line brush executed but no voxels were modified", false);
	}

	return ctx.result(id, "Failed to execute line brush", true);
}

} // namespace voxedit
