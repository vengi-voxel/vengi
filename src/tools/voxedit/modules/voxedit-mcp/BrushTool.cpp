/**
 * @file
 */

#include "BrushTool.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/AABBBrush.h"
#include "voxel/RawVolume.h"

namespace voxedit {

BrushTool::BrushTool(const core::String &name) : Tool(name) {
}

ModifierType BrushTool::parseModifierType(const core::String &type) {
	if (type == "erase") {
		return ModifierType::Erase;
	}
	if (type == "override") {
		return ModifierType::Override;
	}
	if (type == "paint") {
		return ModifierType::Paint;
	}
	return ModifierType::Place;
}

uint32_t BrushTool::parseBrushMode(const core::String &mode) {
	if (mode == "single") {
		return BRUSH_MODE_SINGLE;
	}
	if (mode == "center") {
		return BRUSH_MODE_CENTER;
	}
	return BRUSH_MODE_AABB;
}

nlohmann::json BrushTool::propModifierType() {
	nlohmann::json prop;
	prop["type"] = "string";
	prop["description"] = "The modifier type: 'place' (add voxels), 'erase' (remove voxels), 'override' (replace "
						  "voxels), 'paint' (change color only)";
	prop["enum"] = nlohmann::json::array({"place", "erase", "override", "paint"});
	prop["default"] = "place";
	return prop;
}

nlohmann::json BrushTool::propBrushMode() {
	nlohmann::json prop;
	prop["type"] = "string";
	prop["description"] = "The brush mode: 'aabb' (span rectangular region), 'single' (place single voxels), 'center' "
						  "(expand from center point)";
	prop["enum"] = nlohmann::json::array({"aabb", "single", "center"});
	prop["default"] = "aabb";
	return prop;
}

nlohmann::json BrushTool::propRegion() {
	nlohmann::json prop;
	prop["type"] = "object";
	prop["description"] = "The AABB region to operate on";
	prop["required"] = nlohmann::json::array({"minX", "minY", "minZ", "maxX", "maxY", "maxZ"});
	prop["properties"]["minX"]["type"] = "integer";
	prop["properties"]["minX"]["description"] = "Minimum X coordinate";
	prop["properties"]["minY"]["type"] = "integer";
	prop["properties"]["minY"]["description"] = "Minimum Y coordinate";
	prop["properties"]["minZ"]["type"] = "integer";
	prop["properties"]["minZ"]["description"] = "Minimum Z coordinate";
	prop["properties"]["maxX"]["type"] = "integer";
	prop["properties"]["maxX"]["description"] = "Maximum X coordinate";
	prop["properties"]["maxY"]["type"] = "integer";
	prop["properties"]["maxY"]["description"] = "Maximum Y coordinate";
	prop["properties"]["maxZ"]["type"] = "integer";
	prop["properties"]["maxZ"]["description"] = "Maximum Z coordinate";
	return prop;
}

nlohmann::json BrushTool::propColorIndex() {
	nlohmann::json prop;
	prop["type"] = "integer";
	prop["description"] = "The palette color index (0-255)";
	prop["minimum"] = 0;
	prop["maximum"] = 255;
	prop["default"] = 1;
	return prop;
}

nlohmann::json BrushTool::propPosition(const core::String &description) {
	nlohmann::json prop;
	prop["type"] = "object";
	prop["description"] = description.c_str();
	prop["required"] = nlohmann::json::array({"x", "y", "z"});
	prop["properties"]["x"]["type"] = "integer";
	prop["properties"]["x"]["description"] = "X coordinate";
	prop["properties"]["y"]["type"] = "integer";
	prop["properties"]["y"]["description"] = "Y coordinate";
	prop["properties"]["z"]["type"] = "integer";
	prop["properties"]["z"]["description"] = "Z coordinate";
	return prop;
}

bool BrushTool::executeBrush(ToolContext &ctx, const core::UUID &nodeUUID, BrushType brushType,
							 ModifierType modifierType, int colorIndex, const glm::ivec3 &mins, const glm::ivec3 &maxs,
							 const nlohmann::json &id) {
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

	// Get the brush from the modifier
	Modifier &modifier = ctx.sceneMgr->modifier();
	const BrushType prevBrushType = modifier.brushType();
	modifier.setBrushType(brushType);

	Brush *brush = modifier.currentBrush();
	if (brush == nullptr) {
		modifier.setBrushType(prevBrushType);
		return ctx.result(id, "Failed to get brush", true);
	}

	// For AABB brushes, we need to set up the region using currentAABBBrush()
	if (AABBBrush *aabbBrush = modifier.currentAABBBrush()) {
		aabbBrush->setAABBMode();

		// Manually set up the AABB region by simulating begin/step
		brushContext.cursorPosition = mins;
		aabbBrush->beginBrush(brushContext);

		// Step to the max position
		brushContext.cursorPosition = maxs;
		aabbBrush->step(brushContext);
	}

	// Execute the brush
	brush->preExecute(brushContext, volume);
	const bool success = brush->execute(ctx.sceneMgr->sceneGraph(), wrapper, brushContext);

	// End the brush operation
	brush->endBrush(brushContext);

	// Restore previous brush type
	modifier.setBrushType(prevBrushType);

	const voxel::Region &region = wrapper.dirtyRegion();
	if (region.isValid()) {
		ctx.sceneMgr->modified(node->id(), region);
		return ctx.result(id, "Brush executed successfully", false);
	}

	if (success) {
		return ctx.result(id, "Brush executed but no voxels were modified", false);
	}

	return ctx.result(id, "Failed to execute brush", true);
}

} // namespace voxedit
