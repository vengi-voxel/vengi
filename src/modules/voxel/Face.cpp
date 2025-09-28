/**
 * @file
 */

#include "Face.h"
#include "core/Common.h"
#include <math.h>

#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <glm/common.hpp>

namespace voxel {

FaceNames toFaceNames(const core::String &in, FaceNames defaultFace) {
	core::String lower = in.toLower();
	if (lower.contains("positivex") || lower.contains("east") || lower.contains("right")) {
		return voxel::FaceNames::PositiveX;
	}
	if (lower.contains("negativex") || lower.contains("west") || lower.contains("left")) {
		return voxel::FaceNames::NegativeX;
	}
	if (lower.contains("positivey") || lower.contains("up")) {
		return voxel::FaceNames::PositiveY;
	}
	if (lower.contains("negativey") || lower.contains("down")) {
		return voxel::FaceNames::NegativeY;
	}
	if (lower.contains("positivez") || lower.contains("south") || lower.contains("back")) {
		return voxel::FaceNames::PositiveZ;
	}
	if (lower.contains("negativez") || lower.contains("north") || lower.contains("front")) {
		return voxel::FaceNames::NegativeZ;
	}
	return defaultFace;
}

const char *faceNameString(FaceNames face) {
	switch (face) {
	case voxel::FaceNames::PositiveX:
		return "PositiveX_East_Right";
	case voxel::FaceNames::NegativeX:
		return "NegativeX_West_Left";
	case voxel::FaceNames::PositiveY:
		return "PositiveY_Up";
	case voxel::FaceNames::NegativeY:
		return "NegativeY_Down";
	case voxel::FaceNames::PositiveZ:
		return "PositiveZ_South_Back";
	case voxel::FaceNames::NegativeZ:
		return "NegativeZ_North_Front";
	default:
		break;
	}
	return "Unknown";
}

glm::vec3 faceNormal(FaceNames face) {
	switch (face) {
	case FaceNames::PositiveX:
		return glm::vec3(1.0f, 0.0f, 0.0f);
	case FaceNames::NegativeX:
		return glm::vec3(-1.0f, 0.0f, 0.0f);
	case FaceNames::PositiveY:
		return glm::vec3(0.0f, 1.0f, 0.0f);
	case FaceNames::NegativeY:
		return glm::vec3(0.0f, -1.0f, 0.0f);
	case FaceNames::PositiveZ:
		return glm::vec3(0.0f, 0.0f, 1.0f);
	case FaceNames::NegativeZ:
		return glm::vec3(0.0f, 0.0f, -1.0f);
	default:
		break;
	}
	return glm::vec3(0.0f, 0.0f, 0.0f);
}

math::Axis faceToAxis(FaceNames face) {
	switch (face) {
	case FaceNames::PositiveX:
	case FaceNames::NegativeX:
		return math::Axis::X;
	case FaceNames::PositiveY:
	case FaceNames::NegativeY:
		return math::Axis::Y;
	case FaceNames::PositiveZ:
	case FaceNames::NegativeZ:
		return math::Axis::Z;
	default:
		return math::Axis::X;
	}
}

FaceBits faceBits(FaceNames face) {
	switch (face) {
	case FaceNames::PositiveX:
		return FaceBits::PositiveX;
	case FaceNames::NegativeX:
		return FaceBits::NegativeX;
	case FaceNames::PositiveY:
		return FaceBits::PositiveY;
	case FaceNames::NegativeY:
		return FaceBits::NegativeY;
	case FaceNames::PositiveZ:
		return FaceBits::PositiveZ;
	case FaceNames::NegativeZ:
		return FaceBits::NegativeZ;
	default:
		break;
	}
	return FaceBits::None;
}

} // namespace voxel
