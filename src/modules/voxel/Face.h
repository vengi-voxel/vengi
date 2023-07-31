/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include "voxel/Voxel.h"
#include "voxel/RawVolume.h"
#include <glm/fwd.hpp>

namespace voxel {

enum class FaceNames {
	PositiveX = 0, PositiveY = 1, PositiveZ = 2, NegativeX = 3, NegativeY = 4, NegativeZ = 5, Max
};

inline bool isHorizontalFace(FaceNames face) {
	return face == FaceNames::PositiveX || face == FaceNames::PositiveZ || face == FaceNames::NegativeX || face == FaceNames::NegativeZ;
}

inline bool isNegativeFace(FaceNames face) {
	return face == FaceNames::NegativeX || face == FaceNames::NegativeY || face == FaceNames::NegativeZ;
}

inline bool isPositiveFace(FaceNames face) {
	return !isNegativeFace(face);
}

inline bool isVerticalFace(FaceNames face) {
	 return face == FaceNames::PositiveY || face == FaceNames::NegativeY;
}

enum class FaceBits : uint8_t {
	None = 0,
	PositiveX = 1,
	NegativeX = 2,
	PositiveY = 4,
	NegativeY = 8,
	PositiveZ = 16,
	NegativeZ = 32
};
CORE_ENUM_BIT_OPERATIONS(FaceBits)

template<class Volume>
static FaceBits visibleFaces(const Volume &v, int x, int y, int z) {
	FaceBits vis = FaceBits::None;

	typename Volume::Sampler sampler(v);
	if (!sampler.setPosition(x, y, z)) {
		return vis;
	}
	if (voxel::isAir(sampler.voxel().getMaterial())) {
		return vis;
	}
	if (voxel::isAir(sampler.peekVoxel1px0py0pz().getMaterial())) {
		vis |= FaceBits::PositiveX;
	}
	if (voxel::isAir(sampler.peekVoxel1nx0py0pz().getMaterial())) {
		vis |= FaceBits::NegativeX;
	}
	if (voxel::isAir(sampler.peekVoxel0px1py0pz().getMaterial())) {
		vis |= FaceBits::PositiveY;
	}
	if (voxel::isAir(sampler.peekVoxel0px1ny0pz().getMaterial())) {
		vis |= FaceBits::NegativeY;
	}
	if (voxel::isAir(sampler.peekVoxel0px0py1pz().getMaterial())) {
		vis |= FaceBits::PositiveZ;
	}
	if (voxel::isAir(sampler.peekVoxel0px0py1nz().getMaterial())) {
		vis |= FaceBits::NegativeZ;
	}
	return vis;
}

extern FaceNames raycastFaceDetection(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& hitPos, float offsetMins = -0.5f, float offsetMaxs = 0.5f);
extern FaceNames raycastFaceDetection(const glm::vec3& rayOrigin, const glm::vec3& hitPos, float offsetMins = -0.5f, float offsetMaxs = 0.5f);

}
