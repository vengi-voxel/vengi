/**
 * @file
 */

#pragma once

#include "math/OBB.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Face.h"
#include "voxel/Region.h"
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

namespace voxedit {

struct AddNodePreview {
	bool active = false;
	int sourceNodeId = InvalidNodeId;
	voxel::FaceNames hoverFace = voxel::FaceNames::Max;
	math::OBBF highlightObb = math::OBBF(glm::vec3(0.0f), glm::vec3(1.0f), glm::mat3(1.0f));
	math::OBBF previewObb = math::OBBF(glm::vec3(0.0f), glm::vec3(1.0f), glm::mat3(1.0f));
	voxel::Region previewRegion;
	bool previewValid = false;

	inline void reset() {
		active = false;
		sourceNodeId = InvalidNodeId;
		hoverFace = voxel::FaceNames::Max;
		previewValid = false;
		previewRegion = voxel::Region::InvalidRegion;
	}
};

} // namespace voxedit
