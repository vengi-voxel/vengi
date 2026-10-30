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
	if (mode == "stroke") {
		return BRUSH_MODE_STROKE;
	}
	if (mode == "strokenooverlap") {
		return BRUSH_MODE_STROKE_NO_OVERLAP;
	}
	if (mode == "center") {
		return BRUSH_MODE_CENTER;
	}
	return BRUSH_MODE_BOX;
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
	prop.set("description",
			 "The brush mode: 'box' (span rectangular region), 'stroke' (continuous dabs while held), "
			 "'strokenooverlap' (stroke without redoing the same cell), 'center' (expand from center point)");
	json::Json enumArr = json::Json::array();
	enumArr.push("box");
	enumArr.push("stroke");
	enumArr.push("strokenooverlap");
	enumArr.push("center");
	prop.set("enum", enumArr);
	prop.set("default", "box");
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

} // namespace voxedit
