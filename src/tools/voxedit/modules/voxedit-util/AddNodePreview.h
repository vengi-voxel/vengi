/**
 * @file
 */

#pragma once

#include "math/OBB.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "voxel/Face.h"
#include "voxel/Region.h"
#include "math/Mat3f.h"
#include "math/Vec3f.h"

namespace voxedit {

struct AddNodePreview {
	bool active = false;
	int sourceNodeId = InvalidNodeId;
	voxel::FaceNames hoverFace = voxel::FaceNames::Max;
	math::OBBF highlightObb = math::OBBF(math::Vec3f(0.0f), math::Vec3f(1.0f), math::Mat3f(1.0f));
	math::OBBF previewObb = math::OBBF(math::Vec3f(0.0f), math::Vec3f(1.0f), math::Mat3f(1.0f));
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
