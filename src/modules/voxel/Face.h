/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include "math/Axis.h"
#include "voxel/Voxel.h"
#include <glm/vec3.hpp>

namespace voxel {

enum class FaceNames : uint8_t {
	PositiveX = 0,
	PositiveY = 1,
	PositiveZ = 2,
	NegativeX = 3,
	NegativeY = 4,
	NegativeZ = 5,
	Max,

	Down = NegativeY,
	Up = PositiveY,

	Bottom = NegativeY,
	Top = PositiveY,

	North = NegativeZ,
	South = PositiveZ,
	West = NegativeX,
	East = PositiveX,

	Front = NegativeZ,
	Back = PositiveZ,
	Left = NegativeX,
	Right = PositiveX
};

FaceNames toFaceNames(const core::String &in, FaceNames defaultFace = FaceNames::Max);
const char *faceNameString(FaceNames face);
math::Axis faceToAxis(FaceNames face);
glm::vec3 faceNormal(FaceNames face);

inline bool isHorizontalFace(FaceNames face) {
	return face == FaceNames::PositiveX || face == FaceNames::PositiveZ || face == FaceNames::NegativeX || face == FaceNames::NegativeZ;
}

inline bool isNegativeFace(FaceNames face) {
	return face == FaceNames::NegativeX || face == FaceNames::NegativeY || face == FaceNames::NegativeZ;
}

inline bool isPositiveFace(FaceNames face) {
	return face == FaceNames::PositiveX || face == FaceNames::PositiveY || face == FaceNames::PositiveZ;
}

inline bool isVerticalFace(FaceNames face) {
	 return face == FaceNames::PositiveY || face == FaceNames::NegativeY;
}

inline bool isZ(FaceNames face) {
	 return face == FaceNames::PositiveZ || face == FaceNames::NegativeZ;
}

inline bool isY(FaceNames face) {
	 return face == FaceNames::PositiveY || face == FaceNames::NegativeY;
}

inline bool isX(FaceNames face) {
	 return face == FaceNames::PositiveX || face == FaceNames::NegativeX;
}

enum class FaceBits : uint8_t {
	None = 0,
	PositiveX = 1,
	NegativeX = 2,
	PositiveY = 4,
	NegativeY = 8,
	PositiveZ = 16,
	NegativeZ = 32,

	Down = NegativeY,
	Up = PositiveY,

	North = NegativeZ,
	South = PositiveZ,
	West = NegativeX,
	East = PositiveX,

	Front = NegativeZ,
	Back = PositiveZ,
	Left = NegativeX,
	Right = PositiveX,

	All = PositiveX | NegativeX | PositiveY | NegativeY | PositiveZ | NegativeZ
};
CORE_ENUM_BIT_OPERATIONS(FaceBits)

FaceBits faceBits(FaceNames face);

template<class Sampler>
static FaceBits visibleFaces(const Sampler &sampler, bool skipEmpty = true) {
	FaceBits vis = FaceBits::None;
	if (skipEmpty && voxel::isAir(sampler.voxel().getMaterial())) {
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

template<class Volume>
static FaceBits visibleFaces(const Volume &v, int x, int y, int z, bool skipEmpty = true) {
	typename Volume::Sampler sampler(v);
	if (!sampler.setPosition(x, y, z)) {
		return FaceBits::None;
	}
	return visibleFaces(sampler, skipEmpty);
}

template<class Volume>
static FaceBits visibleFaces(const Volume &v, const glm::ivec3 &pos, bool skipEmpty = true) {
	return visibleFaces(v, pos.x, pos.y, pos.z, skipEmpty);
}

}
