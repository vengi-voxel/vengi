/**
 * @file
 */

#pragma once

#include "app/I18NMarkers.h"
#include "core/ArrayLength.h"
#include <stdint.h>

namespace scenegraph {

enum class SceneGraphNodeType : uint8_t {
	Root,
	Model,
	ModelReference,
	Group,
	Camera,
	Point, // a point in space with a transform and a name
	Unknown,

	Max,

	AllModels, // Fake type for the iterator
	All
};

// these identifiers are using in the vengi format for the different node types
// if you change these, VENGIFormat might need a migration path
static constexpr const char *SceneGraphNodeTypeStr[]{
	NC_("SceneGraphNodeType", "Root"),			 NC_("SceneGraphNodeType", "Model"),
	NC_("SceneGraphNodeType", "ModelReference"), NC_("SceneGraphNodeType", "Group"),
	NC_("SceneGraphNodeType", "Camera"),		 NC_("SceneGraphNodeType", "Point"),
	NC_("SceneGraphNodeType", "Unknown"),		 NC_("SceneGraphNodeType", "None"),
	NC_("SceneGraphNodeType", "AllModels"),		 NC_("SceneGraphNodeType", "All")};
static_assert((int)(scenegraph::SceneGraphNodeType::Max) + 3 == lengthof(SceneGraphNodeTypeStr),
			  "Array sizes don't match Max");

}; // namespace scenegraph
