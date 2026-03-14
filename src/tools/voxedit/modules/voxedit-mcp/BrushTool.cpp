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

json::Json BrushTool::propModifierType() {
	json::Json prop = json::Json::object();
	prop.set("type", "string");
	prop.set("description", "The modifier type: 'place' (add voxels), 'erase' (remove voxels), 'override' (replace "
						  "voxels), 'paint' (change color only)");
	json::Json enumArr = json::Json::array();
	enumArr.push("place");
	enumArr.push("erase");
	enumArr.push("override");
	enumArr.push("paint");
	prop.set("enum", enumArr);
	prop.set("default", "place");
	return prop;
}

json::Json BrushTool::propBrushMode() {
	json::Json prop = json::Json::object();
	prop.set("type", "string");
	prop.set("description", "The brush mode: 'aabb' (span rectangular region), 'single' (place single voxels), 'center' "
						  "(expand from center point)");
	json::Json enumArr = json::Json::array();
	enumArr.push("aabb");
	enumArr.push("single");
	enumArr.push("center");
	prop.set("enum", enumArr);
	prop.set("default", "aabb");
	return prop;
}

json::Json BrushTool::propRegion() {
	json::Json prop = json::Json::object();
	prop.set("type", "object");
	prop.set("description", "The AABB region to operate on");
	json::Json required = json::Json::array();
	required.push("minX");
	required.push("minY");
	required.push("minZ");
	required.push("maxX");
	required.push("maxY");
	required.push("maxZ");
	prop.set("required", required);
	json::Json properties = json::Json::object();
	properties.set("minX", propTypeDescription("integer", "Minimum X coordinate"));
	properties.set("minY", propTypeDescription("integer", "Minimum Y coordinate"));
	properties.set("minZ", propTypeDescription("integer", "Minimum Z coordinate"));
	properties.set("maxX", propTypeDescription("integer", "Maximum X coordinate"));
	properties.set("maxY", propTypeDescription("integer", "Maximum Y coordinate"));
	properties.set("maxZ", propTypeDescription("integer", "Maximum Z coordinate"));
	prop.set("properties", properties);
	return prop;
}

json::Json BrushTool::propColorIndex() {
	json::Json prop = json::Json::object();
	prop.set("type", "integer");
	prop.set("description", "The palette color index (0-255)");
	prop.set("minimum", 0);
	prop.set("maximum", 255);
	prop.set("default", 1);
	return prop;
}

json::Json BrushTool::propPosition(const core::String &description) {
	json::Json prop = json::Json::object();
	prop.set("type", "object");
	prop.set("description", description.c_str());
	json::Json required = json::Json::array();
	required.push("x");
	required.push("y");
	required.push("z");
	prop.set("required", required);
	json::Json properties = json::Json::object();
	properties.set("x", propTypeDescription("integer", "X coordinate"));
	properties.set("y", propTypeDescription("integer", "Y coordinate"));
	properties.set("z", propTypeDescription("integer", "Z coordinate"));
	prop.set("properties", properties);
	return prop;
}

bool BrushTool::executeBrush(ToolContext &ctx, const core::UUID &nodeUUID, BrushType brushType,
							 ModifierType modifierType, int colorIndex, const glm::ivec3 &mins, const glm::ivec3 &maxs,
							 const json::Json &id) {
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
