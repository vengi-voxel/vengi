/**
 * @file
 */

#include "VoxelUtil.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/collection/Set.h"
#include "math/Axis.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "voxel/Face.h"
#include "voxel/ModificationRecorder.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeRotator.h"
#include "voxelutil/VolumeVisitor.h"
#include <functional>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

namespace voxelutil {

voxel::RawVolume *applyTransformToVolume(const voxel::RawVolume &volume, const glm::mat4 &worldMat,
										 const glm::vec3 &normalizedPivot) {
	// TODO: scaling is not applied properly
	const glm::ivec3 translation(worldMat[3]);
	glm::quat q = glm::quat_cast(worldMat);
	const glm::vec3 angles = glm::degrees(glm::eulerAngles(q));
	Log::debug("Apply transforms: angles: %f %f %f, translation: %i %i %i", angles.x, angles.y, angles.z, translation.x,
			   translation.y, translation.z);
	if (glm::all(glm::epsilonEqual(angles, glm::vec3(0.0f), 0.001f))) {
		voxel::RawVolume *v = new voxel::RawVolume(volume);
		v->translate(translation);
		return v;
	}
	glm::mat4 rotMat = glm::mat4_cast(q);
	voxel::RawVolume *rotated = voxelutil::rotateVolume(&volume, rotMat, normalizedPivot);
	rotated->translate(translation);
	return rotated;
}

bool isTouching(const voxel::RawVolume &volume, const glm::ivec3 &pos, voxel::Connectivity connectivity) {
	voxel::RawVolume::Sampler sampler(volume);
	if (!sampler.setPosition(pos)) {
		return false;
	}
	switch (connectivity) {
	case voxel::Connectivity::TwentySixConnected:
		for (const glm::ivec3 &offset : voxel::arrayPathfinderCorners) {
			const glm::ivec3 &volPos = pos + offset;
			if (!sampler.setPosition(volPos)) {
				continue;
			}
			if (voxel::isBlocked(sampler.voxel().getMaterial())) {
				return true;
			}
		}
		/* fallthrough */

	case voxel::Connectivity::EighteenConnected:
		for (const glm::ivec3 &offset : voxel::arrayPathfinderEdges) {
			const glm::ivec3 &volPos = pos + offset;
			if (!sampler.setPosition(volPos)) {
				continue;
			}
			if (voxel::isBlocked(sampler.voxel().getMaterial())) {
				return true;
			}
		}
		/* fallthrough */

	case voxel::Connectivity::SixConnected:
		for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
			const glm::ivec3 &volPos = pos + offset;
			if (!sampler.setPosition(volPos)) {
				continue;
			}
			if (voxel::isBlocked(sampler.voxel().getMaterial())) {
				return true;
			}
		}
		break;
	}
	return false;
}

bool isEmpty(const voxel::RawVolume &v, const voxel::Region &region) {
	voxel::RawVolume::Sampler sampler(v);
	for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += 1) {
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += 1) {
			sampler.setPosition(x, y, region.getLowerZ());
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += 1) {
				if (voxel::isBlocked(sampler.voxel().getMaterial())) {
					return false;
				}
				sampler.movePositiveZ();
			}
		}
	}
	return true;
}

void fill(voxel::RawVolumeWrapper &volume, const voxel::Voxel &voxel, bool overwrite) {
	if (overwrite) {
		volume.fill(voxel);
		return;
	}
	auto visitor = [&](int x, int y, int z, const voxel::Voxel &) { volume.setVoxel(x, y, z, voxel); };
	visitVolumeParallel(volume, visitor, VisitEmpty());
}

void clear(voxel::RawVolumeWrapper &in) {
	in.clear();
}

using IVec3Set = core::Set<glm::ivec3, 11, glm::hash<glm::ivec3>>;
using WalkCheckCallback = std::function<bool(const voxel::RawVolumeWrapper &, const glm::ivec3 &, voxel::FaceNames)>;
using WalkExecCallback = std::function<bool(voxel::RawVolumeWrapper &, const glm::ivec3 &)>;

template<class CHECK, class EXEC, class Volume>
static int walkPlane_r(IVec3Set &visited, Volume &volume, const voxel::Region &region, CHECK &&check, EXEC &&exec,
					   const glm::ivec3 &position, const glm::ivec3 &offsetForCheckCallback, voxel::FaceNames face) {
	const glm::ivec3 checkPosition = position + offsetForCheckCallback;
	if (visited.has(checkPosition)) {
		return 0;
	}
	visited.insert(checkPosition);
	if (!region.containsPoint(position)) {
		return 0;
	}
	if (!check(volume, checkPosition, face)) {
		return 0;
	}
	if (!exec(volume, position)) {
		return 0;
	}
	int n = 1;
	if (region.containsPoint(position.x + 1, position.y, position.z)) {
		n += walkPlane_r(visited, volume, region, check, exec, glm::ivec3(position.x + 1, position.y, position.z),
						 offsetForCheckCallback, face);
	}
	if (region.containsPoint(position.x - 1, position.y, position.z)) {
		n += walkPlane_r(visited, volume, region, check, exec, glm::ivec3(position.x - 1, position.y, position.z),
						 offsetForCheckCallback, face);
	}
	if (region.containsPoint(position.x, position.y + 1, position.z)) {
		n += walkPlane_r(visited, volume, region, check, exec, glm::ivec3(position.x, position.y + 1, position.z),
						 offsetForCheckCallback, face);
	}
	if (region.containsPoint(position.x, position.y - 1, position.z)) {
		n += walkPlane_r(visited, volume, region, check, exec, glm::ivec3(position.x, position.y - 1, position.z),
						 offsetForCheckCallback, face);
	}
	if (region.containsPoint(position.x, position.y, position.z + 1)) {
		n += walkPlane_r(visited, volume, region, check, exec, glm::ivec3(position.x, position.y, position.z + 1),
						 offsetForCheckCallback, face);
	}
	if (region.containsPoint(position.x, position.y, position.z - 1)) {
		n += walkPlane_r(visited, volume, region, check, exec, glm::ivec3(position.x, position.y, position.z - 1),
						 offsetForCheckCallback, face);
	}
	return n;
}

/**
 * @brief Walks a plane in a voxel volume based on the given position and face direction.
 *
 * @param volume The voxel volume to walk the plane in.
 * @param position The position in the voxel volume to start the walk from.
 * @param face The direction of the face to walk the plane in.
 * @param checkOffset The offset for checking voxel positions. This offset it applied to the check callback in the
 * correct direction e.g. one above or below (according to the face)
 * @param checkCallback The callback function to use for checking voxel conditions - if @c true is returned, it's a
 * valid position to check the neighbor for the next steps.
 * @param execCallback The callback function to use for executing operations on the voxels. Only executed if @c check
 * returned
 * @c true
 */
template<class CHECK, class EXEC, class Volume>
static int walkPlane(Volume &volume, glm::ivec3 position, voxel::FaceNames face, int checkOffset, CHECK &&checkCallback,
					 EXEC &&execCallback, int amount) {
	const math::Axis axis = voxel::faceToAxis(face);
	if (axis == math::Axis::None) {
		return -1;
	}
	const voxel::Region &region = volume.region();
	const int idx = math::getIndexForAxis(axis);
	const bool negativeFace = voxel::isNegativeFace(face);

	glm::ivec3 mins = region.getLowerCorner();
	glm::ivec3 maxs = region.getUpperCorner();
	mins[idx] = position[idx];
	maxs[idx] = position[idx];

	// which voxel should we check on
	glm::ivec3 offsetForCheckCallback(0);
	if (negativeFace) {
		offsetForCheckCallback[idx] = -checkOffset;
	} else {
		offsetForCheckCallback[idx] = checkOffset;
	}

	const int walkOffset = negativeFace ? -1 : 1;

	int n = 0;
	for (int i = 0; i < amount; ++i) {
		const voxel::Region walkRegion(mins, maxs);
		if (!walkRegion.isValid()) {
			return 0;
		}
		const glm::ivec3 &dim = walkRegion.getDimensionsInVoxels();
		const int maxSize = dim.x * dim.y * dim.z;
		IVec3Set visited(maxSize);
		const int n0 = walkPlane_r(visited, volume, walkRegion, checkCallback, execCallback, position, offsetForCheckCallback, face);
		if (n0 == 0) {
			break;
		}
		mins[idx] += walkOffset;
		maxs[idx] += walkOffset;
		position[idx] += walkOffset;
		n += n0;
	}
	return n;
}

static glm::vec2 calcUV(const glm::ivec3 &pos, const voxel::Region &region, voxel::FaceNames face) {
	const glm::ivec3 &dim = region.getDimensionsInVoxels();
	switch (face) {
	case voxel::FaceNames::PositiveX:
	case voxel::FaceNames::NegativeX:
		return image::Image::uv(pos.z, region.getHeightInCells() - pos.y, dim.z, dim.y);
	case voxel::FaceNames::PositiveY:
	case voxel::FaceNames::NegativeY:
		return image::Image::uv(pos.x, pos.z, dim.x, dim.z);
	case voxel::FaceNames::PositiveZ:
	case voxel::FaceNames::NegativeZ:
		return image::Image::uv(pos.x, region.getHeightInCells() - pos.y, dim.x, dim.y);
	default:
	case voxel::FaceNames::Max:
		return glm::vec2(0.0f);
	}
}

template<class Volume>
static bool checkOverrideFunc(Volume &volume, const glm::ivec3 &pos, const voxel::Voxel &replaceVoxel,
							  voxel::FaceNames face, bool &firstVoxelIsAir, bool &firstVoxel) {
	const voxel::Voxel &v = volume.voxel(pos);
	if (firstVoxel) {
		firstVoxelIsAir = voxel::isAir(v.getMaterial());
		firstVoxel = false;
	}
	if (firstVoxelIsAir) {
		return voxel::isAir(v.getMaterial());
	}
	return v.isSame(replaceVoxel);
}

int overridePlane(voxel::RawVolumeWrapper &volume, const glm::ivec3 &pos, voxel::FaceNames face,
				  const voxel::Voxel &replaceVoxel, int thickness) {
	bool firstVoxelIsAir = false;
	bool firstVoxel = true;
	auto check = [&](const voxel::RawVolumeWrapper &in, const glm::ivec3 &p, voxel::FaceNames) {
		return checkOverrideFunc(in, p, replaceVoxel, face, firstVoxelIsAir, firstVoxel);
	};
	auto exec = [=](voxel::RawVolumeWrapper &in, const glm::ivec3 &p) { return in.setVoxel(p, replaceVoxel); };
	return voxelutil::walkPlane(volume, pos, voxel::oppositeFace(face), -1, check, exec, thickness);
}

voxel::Region overridePlaneRegion(const voxel::RawVolume &volume, const glm::ivec3 &pos, voxel::FaceNames face,
								  const voxel::Voxel &replaceVoxel, int thickness) {
	bool firstVoxelIsAir = false;
	bool firstVoxel = true;
	auto check = [&](const voxel::ModificationRecorder &in, const glm::ivec3 &p, voxel::FaceNames) {
		return checkOverrideFunc(in, p, replaceVoxel, face, firstVoxelIsAir, firstVoxel);
	};
	auto exec = [=](voxel::ModificationRecorder &in, const glm::ivec3 &p) { return in.setVoxel(p, replaceVoxel); };
	voxel::ModificationRecorder recorder(volume);
	voxelutil::walkPlane(recorder, pos, voxel::oppositeFace(face), -1, check, exec, thickness);
	return recorder.dirtyRegion();
}

int paintPlane(voxel::RawVolumeWrapper &volume, const glm::ivec3 &pos, voxel::FaceNames face,
			   const voxel::Voxel &searchVoxel, const voxel::Voxel &replaceVoxel) {
	auto check = [&](const voxel::RawVolumeWrapper &in, const glm::ivec3 &p, voxel::FaceNames) {
		const voxel::Voxel &v = in.voxel(p);
		return v.isSame(searchVoxel);
	};
	auto exec = [=](voxel::RawVolumeWrapper &in, const glm::ivec3 &p) {
		return in.setVoxel(p, replaceVoxel);
	};
	return voxelutil::walkPlane(volume, pos, face, 0, check, exec, 1);
}

template<class Volume>
static bool checkEraseFunc(Volume &volume, const glm::ivec3 &p, const voxel::Voxel &groundVoxel,
						   voxel::FaceNames face) {
	const voxel::Voxel &voxel = volume.voxel(p);
	if (voxel.isSame(groundVoxel)) {
		const math::Axis axis = voxel::faceToAxis(face);
		const int idx = math::getIndexForAxis(axis);
		const int offset = voxel::isNegativeFace(face) ? -1 : 1;
		glm::ivec3 abovePos = p;
		abovePos[idx] += offset;
		const voxel::Voxel &aboveVoxel = volume.voxel(abovePos);
		return voxel::isAir(aboveVoxel.getMaterial());
	}
	return false;
}

int erasePlane(voxel::RawVolumeWrapper &volume, const glm::ivec3 &pos, voxel::FaceNames face,
			   const voxel::Voxel &groundVoxel, int thickness) {
	auto check = [&](const voxel::RawVolumeWrapper &in, const glm::ivec3 &p, voxel::FaceNames) {
		return checkEraseFunc(in, p, groundVoxel, face);
	};
	auto exec = [](voxel::RawVolumeWrapper &in, const glm::ivec3 &p) { return in.setVoxel(p, voxel::Voxel()); };
	return voxelutil::walkPlane(volume, pos, voxel::oppositeFace(face), 0, check, exec, thickness);
}

voxel::Region erasePlaneRegion(const voxel::RawVolume &volume, const glm::ivec3 &pos, voxel::FaceNames face,
							   const voxel::Voxel &groundVoxel, int thickness) {
	auto check = [&](const voxel::ModificationRecorder &in, const glm::ivec3 &p, voxel::FaceNames) {
		return checkEraseFunc(in, p, groundVoxel, face);
	};
	auto exec = [](voxel::ModificationRecorder &in, const glm::ivec3 &p) { return in.setVoxel(p, voxel::Voxel()); };
	voxel::ModificationRecorder recorder(volume);
	voxelutil::walkPlane(recorder, pos, voxel::oppositeFace(face), 0, check, exec, thickness);
	return recorder.dirtyRegion();
}

template<class Volume>
static bool checkExtrudeFunc(Volume &volume, const glm::ivec3 &callbackPos, voxel::FaceNames direction,
							 const glm::ivec3 &initialCursorPos, const voxel::Voxel &groundVoxel,
							 const voxel::Voxel &newPlaneVoxel) {
	const voxel::Voxel &v = volume.voxel(callbackPos);
	const math::Axis axis = voxel::faceToAxis(direction);
	const int idx = math::getIndexForAxis(axis);
	const int offset = voxel::isNegativeFace(direction) ? -1 : 1;
	if (callbackPos[idx] + offset == initialCursorPos[idx]) {
		return v.isSame(groundVoxel);
	}
	return v.isSame(newPlaneVoxel);
}

voxel::Region extrudePlaneRegion(const voxel::RawVolume &volume, const glm::ivec3 &pos, voxel::FaceNames face,
								 const voxel::Voxel &groundVoxel, const voxel::Voxel &newPlaneVoxel, int thickness) {
	auto check = [&](voxel::ModificationRecorder &in, const glm::ivec3 &p, voxel::FaceNames direction) {
		return checkExtrudeFunc(in, p, direction, pos, groundVoxel, newPlaneVoxel);
	};

	auto exec = [&](voxel::ModificationRecorder &in, const glm::ivec3 &p) {
		in.setVoxel(p, newPlaneVoxel);
		return true;
	};
	voxel::ModificationRecorder recorder(volume);
	voxelutil::walkPlane(recorder, pos, face, -1, check, exec, thickness);
	return recorder.dirtyRegion();
}

int extrudePlane(voxel::RawVolumeWrapper &volume, const glm::ivec3 &pos, voxel::FaceNames face,
				 const voxel::Voxel &groundVoxel, const voxel::Voxel &newPlaneVoxel, int thickness) {
	auto check = [&](const voxel::RawVolumeWrapper &in, const glm::ivec3 &p, voxel::FaceNames direction) {
		return checkExtrudeFunc(in, p, direction, pos, groundVoxel, newPlaneVoxel);
	};
	auto exec = [=](voxel::RawVolumeWrapper &in, const glm::ivec3 &p) { return in.setVoxel(p, newPlaneVoxel); };
	return voxelutil::walkPlane(volume, pos, face, -1, check, exec, thickness);
}

int fillPlane(voxel::RawVolumeWrapper &volume, const image::ImagePtr &image, const voxel::Voxel &searchedVoxel,
			  const glm::ivec3 &position, voxel::FaceNames face) {
	palette::Palette pal;
	pal.nippon();
	palette::PaletteLookup palLookup(pal);

	const voxel::Region &region = volume.region();

	auto check = [searchedVoxel](const voxel::RawVolumeWrapper &in, const glm::ivec3 &p, voxel::FaceNames) {
		if (voxel::isAir(searchedVoxel.getMaterial())) {
			return true;
		}
		const voxel::Voxel &voxel = in.voxel(p);
		return voxel.isSame(searchedVoxel);
	};

	auto exec = [&](voxel::RawVolumeWrapper &in, const glm::ivec3 &p) {
		const glm::vec2 &uv = calcUV(p, region, face);
		const color::RGBA rgba = image->colorAt(uv);
		if (rgba.a == 0) {
			return true;
		}
		const uint8_t index = palLookup.findClosestIndex(rgba);
		voxel::Voxel voxel = voxel::createVoxel(palLookup.palette(), index);
		return in.setVoxel(p, voxel);
	};

	return walkPlane(volume, position, face, -1, check, exec, 1);
}

voxel::Region remapToPalette(voxel::RawVolume *volume, const palette::Palette &oldPalette,
							 const palette::Palette &newPalette, int skipColorIndex) {
	if (volume == nullptr) {
		return voxel::Region::InvalidRegion;
	}
	voxel::RawVolumeWrapper wrapper(volume);
	auto func = [&wrapper, &newPalette, skipColorIndex, &oldPalette](int x, int y, int z, const voxel::Voxel &voxel) {
		const color::RGBA rgba = oldPalette.color(voxel.getColor());
		const int newColor = newPalette.getClosestMatch(rgba, skipColorIndex);
		if (newColor != palette::PaletteColorNotFound) {
			voxel::Voxel newVoxel(voxel::VoxelType::Generic, newColor, voxel.getNormal(), voxel.getFlags());
			wrapper.setVoxel(x, y, z, newVoxel);
		}
	};
	voxelutil::visitVolumeParallel(wrapper, func);
	return wrapper.dirtyRegion();
}

voxel::RawVolume *diffVolumes(const voxel::RawVolume *v1, const voxel::RawVolume *v2) {
	const voxel::Region &r1 = v1->region();
	const voxel::Region &r2 = v2->region();

	const glm::ivec3 &r1mins = r1.getLowerCorner();
	const glm::ivec3 &r1maxs = r1.getUpperCorner();
	const glm::ivec3 &r2mins = r2.getLowerCorner();
	const glm::ivec3 &r2maxs = r2.getUpperCorner();
	voxel::RawVolume *v = nullptr;
	// TODO: PERF: use a sampler
	for (int r1z = r1mins.z, r2z = r2mins.z; r1z <= r1maxs.z && r2z <= r2maxs.z; ++r1z, ++r2z) {
		for (int r1y = r1mins.y, r2y = r2mins.y; r1y <= r1maxs.y && r2y <= r2maxs.y; ++r1y, ++r2y) {
			for (int r1x = r1mins.x, r2x = r2mins.x; r1x <= r1maxs.x && r2x <= r2maxs.x; ++r1x, ++r2x) {
				const voxel::Voxel &vox1 = v1->voxel(r1x, r1y, r1z);
				voxel::Voxel vox2 = v2->voxel(r2x, r2y, r2z);
				if (vox1.isSame(vox2)) {
					continue;
				}

				if (voxel::isAir(vox2.getMaterial())) {
					vox2 = voxel::createVoxel(voxel::VoxelType::Generic, 1);
					Log::info("Voxel at %i/%i/%i is air", r1x, r1y, r1z);
				}

				if (v == nullptr) {
					v = new voxel::RawVolume(r1);
				}
				v->setVoxel(r1x, r1y, r1z, vox2);
			}
		}
	}
	return v;
}

} // namespace voxelutil
