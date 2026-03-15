/**
 * @file
 */

#pragma once

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
static constexpr const char *SceneGraphNodeTypeStr[]{"Root",   "Model", "ModelReference", "Group",
													 "Camera", "Point", "Unknown",		  "None"};
static_assert((int)(scenegraph::SceneGraphNodeType::Max) + 1 == lengthof(SceneGraphNodeTypeStr),
			  "Array sizes don't match Max");

}; // namespace scenegraph
