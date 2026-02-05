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
	_tool["description"] = "Draw a straight line of voxels between two positions";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"nodeUUID", "start", "end"});

	nlohmann::json properties;
	properties["nodeUUID"] = propUUID();
	properties["start"] = propPosition("The starting position of the line");
	properties["end"] = propPosition("The ending position of the line");
	properties["colorIndex"] = propColorIndex();
	properties["modifierType"] = propModifierType();

	// Continuous mode property
	nlohmann::json continuousProp;
	continuousProp["type"] = "boolean";
	continuousProp["description"] = "If true, the end position becomes the start of the next line segment";
	continuousProp["default"] = false;
	properties["continuous"] = continuousProp;

	inputSchema["properties"] = core::move(properties);
	_tool["inputSchema"] = core::move(inputSchema);
}

bool LineBrushTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
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

	const nlohmann::json &startPos = args["start"];
	const nlohmann::json &endPos = args["end"];

	const glm::ivec3 start(startPos.value("x", 0), startPos.value("y", 0), startPos.value("z", 0));
	const glm::ivec3 end(endPos.value("x", 0), endPos.value("y", 0), endPos.value("z", 0));

	const int colorIndex = args.value("colorIndex", 0);
	const core::String modifierTypeStr = args.value("modifierType", "place").c_str();
	const ModifierType modifierType = parseModifierType(modifierTypeStr);
	const bool continuous = args.value("continuous", false);

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
