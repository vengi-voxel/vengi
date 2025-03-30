/**
 * @file
 */

#include "VoxelUtil.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/collection/Array3DView.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Set.h"
#include <glm/geometric.hpp>
#include "math/Axis.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "voxel/Face.h"
#include "voxel/ModificationRecorder.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include <functional>

namespace voxelutil {

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

voxel::Voxel getInterpolated(const voxel::RawVolumeWrapper &volume, const glm::ivec3 &pos, const palette::Palette &palette) {
	voxel::RawVolumeWrapper::Sampler sampler3(volume);
	sampler3.setPosition(pos);
	const voxel::Voxel voxel000 = sampler3.peekVoxel0px0py0pz();
	const voxel::Voxel voxel001 = sampler3.peekVoxel0px0py1pz();
	const voxel::Voxel voxel010 = sampler3.peekVoxel0px1py0pz();
	const voxel::Voxel voxel011 = sampler3.peekVoxel0px1py1pz();
	const voxel::Voxel voxel100 = sampler3.peekVoxel1px0py0pz();
	const voxel::Voxel voxel101 = sampler3.peekVoxel1px0py1pz();
	const voxel::Voxel voxel110 = sampler3.peekVoxel1px1py0pz();
	const voxel::Voxel voxel111 = sampler3.peekVoxel1px1py1pz();

	const bool blocked000 = voxel::isBlocked(voxel000.getMaterial());
	const bool blocked001 = voxel::isBlocked(voxel001.getMaterial());
	const bool blocked010 = voxel::isBlocked(voxel010.getMaterial());
	const bool blocked011 = voxel::isBlocked(voxel011.getMaterial());
	const bool blocked100 = voxel::isBlocked(voxel100.getMaterial());
	const bool blocked101 = voxel::isBlocked(voxel101.getMaterial());
	const bool blocked110 = voxel::isBlocked(voxel110.getMaterial());
	const bool blocked111 = voxel::isBlocked(voxel111.getMaterial());

	const int blocked = (int)blocked000 + (int)blocked001 + (int)blocked010 + (int)blocked011 +
						(int)blocked100 + (int)blocked101 + (int)blocked110 + (int)blocked111;
	if (blocked == 0) {
		return voxel::Voxel();
	}

	const glm::vec4 color000(blocked000 ? palette.color4(voxel000.getColor()) : glm::vec4(0.0f));
	const glm::vec4 color001(blocked001 ? palette.color4(voxel001.getColor()) : glm::vec4(0.0f));
	const glm::vec4 color010(blocked010 ? palette.color4(voxel010.getColor()) : glm::vec4(0.0f));
	const glm::vec4 color011(blocked011 ? palette.color4(voxel011.getColor()) : glm::vec4(0.0f));
	const glm::vec4 color100(blocked100 ? palette.color4(voxel100.getColor()) : glm::vec4(0.0f));
	const glm::vec4 color101(blocked101 ? palette.color4(voxel101.getColor()) : glm::vec4(0.0f));
	const glm::vec4 color110(blocked110 ? palette.color4(voxel110.getColor()) : glm::vec4(0.0f));
	const glm::vec4 color111(blocked111 ? palette.color4(voxel111.getColor()) : glm::vec4(0.0f));
	const glm::vec4 colorSum =
		color000 + color001 + color010 + color011 + color100 + color101 + color110 + color111;
	const glm::vec4 colorAvg = colorSum / (float)blocked;
	const int idx = palette.getClosestMatch(core::Color::getRGBA(colorAvg));
	if (idx == palette::PaletteColorNotFound) {
		return voxel::Voxel();
	}
	return voxel::createVoxel(palette, idx);
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

bool copy(const voxel::RawVolume &volume, const voxel::Region &inRegion, voxel::RawVolume &out,
		  const voxel::Region &outRegion) {
	int32_t xIn, yIn, zIn;
	int32_t xOut, yOut, zOut;
	voxel::RawVolumeWrapper wrapper(&out);
	const glm::ivec3 &inmins = inRegion.getLowerCorner();
	const glm::ivec3 &inmaxs = inRegion.getUpperCorner();
	const glm::ivec3 &outmins = outRegion.getLowerCorner();
	const glm::ivec3 &outmaxs = outRegion.getUpperCorner();
	for (zIn = inmins.z, zOut = outmins.z; zIn <= inmaxs.z && zOut <= outmaxs.z; ++zIn, ++zOut) {
		for (yIn = inmins.y, yOut = outmins.y; yIn <= inmaxs.y && yOut <= outmaxs.y; ++yIn, ++yOut) {
			for (xIn = inmins.x, xOut = outmins.x; xIn <= inmaxs.x && xOut <= outmaxs.x; ++xIn, ++xOut) {
				const voxel::Voxel &voxel = volume.voxel(xIn, yIn, zIn);
				wrapper.setVoxel(xOut, yOut, zOut, voxel);
			}
		}
	}
	return wrapper.dirtyRegion().isValid();
}

bool copyIntoRegion(const voxel::RawVolume &in, voxel::RawVolume &out, const voxel::Region &targetRegion) {
	return copy(in, in.region(), out, targetRegion);
}

void fillHollow(voxel::RawVolumeWrapper &volume, const voxel::Voxel &voxel) {
	const voxel::Region &region = volume.region();
	const int width = region.getWidthInVoxels();
	const int height = region.getHeightInVoxels();
	const int depth = region.getDepthInVoxels();
	const int size = width * height * depth;
	const glm::ivec3 mins = volume.region().getLowerCorner();
	core::DynamicArray<glm::ivec3> positions;
	positions.reserve(size);
	core::Buffer<bool> visitedData(size);
	core::Array3DView<bool> visited(visitedData.data(), width, height, depth);

	visitVolume(
		volume, region, 1, 1, 1,
		[&](int x, int y, int z, const voxel::Voxel &) { visited.set(x - mins.x, y - mins.y, z - mins.z, true); },
		SkipEmpty());

	for (int x = 0; x < width; ++x) {
		for (int z = 1; z < depth - 1; ++z) {
			const glm::ivec3 v1(x, 0, z);
			const voxel::VoxelType m1 = volume.voxel(v1 + mins).getMaterial();
			if (voxel::isAir(m1) || voxel::isTransparent(m1)) {
				positions.push_back(v1);
				visited.set(v1, true);
			}
			const glm::ivec3 v2(x, height - 1, z);
			const voxel::VoxelType m2 = volume.voxel(v2 + mins).getMaterial();
			if (voxel::isAir(m2) || voxel::isTransparent(m2)) {
				positions.push_back(v2);
				visited.set(v2, true);
			}
		}
		for (int y = 0; y < height; ++y) {
			const glm::ivec3 v1(x, y, 0);
			const voxel::VoxelType m1 = volume.voxel(v1 + mins).getMaterial();
			if (voxel::isAir(m1) || voxel::isTransparent(m1)) {
				positions.push_back(v1);
				visited.set(v1, true);
			}
			const glm::ivec3 v2(x, y, depth - 1);
			const voxel::VoxelType m2 = volume.voxel(v2 + mins).getMaterial();
			if (voxel::isAir(m2) || voxel::isTransparent(m2)) {
				positions.push_back(v2);
				visited.set(v2, true);
			}
		}
	}
	for (int y = 1; y < height - 1; ++y) {
		for (int z = 1; z < depth - 1; ++z) {
			const glm::ivec3 v1(0, y, z);
			const voxel::VoxelType m1 = volume.voxel(v1 + mins).getMaterial();
			if (voxel::isAir(m1) || voxel::isTransparent(m1)) {
				positions.push_back(v1);
				visited.set(v1, true);
			}
			const glm::ivec3 v2(width - 1, y, z);
			const voxel::VoxelType m2 = volume.voxel(v2 + mins).getMaterial();
			if (voxel::isAir(m2) || voxel::isTransparent(m2)) {
				positions.push_back(v2);
				visited.set(v2, true);
			}
		}
	}

	while (!positions.empty()) {
		const glm::ivec3 v = positions.back();
		positions.erase(positions.size() - 1);

		if (v.z > 0 && !visited.get(v.x, v.y, v.z - 1)) {
			const glm::ivec3 v1(v.x, v.y, v.z - 1);
			visited.set(v1, true);
			positions.push_back(v1);
		}
		if (v.y > 0 && !visited.get(v.x, v.y - 1, v.z)) {
			const glm::ivec3 v1(v.x, v.y - 1, v.z);
			visited.set(v1, true);
			positions.push_back(v1);
		}
		if (v.x > 0 && !visited.get(v.x - 1, v.y, v.z)) {
			const glm::ivec3 v1(v.x - 1, v.y, v.z);
			visited.set(v1, true);
			positions.push_back(v1);
		}
		if (v.z < depth - 1 && !visited.get(v.x, v.y, v.z + 1)) {
			const glm::ivec3 v1(v.x, v.y, v.z + 1);
			visited.set(v1, true);
			positions.push_back(v1);
		}
		if (v.y < height - 1 && !visited.get(v.x, v.y + 1, v.z)) {
			const glm::ivec3 v1(v.x, v.y + 1, v.z);
			visited.set(v1, true);
			positions.push_back(v1);
		}
		if (v.x < width - 1 && !visited.get(v.x + 1, v.y, v.z)) {
			const glm::ivec3 v1(v.x + 1, v.y, v.z);
			visited.set(v1, true);
			positions.push_back(v1);
		}
	}

	auto visitor = [&](int x, int y, int z, const voxel::Voxel &) {
		if (!visited.get(x - mins.x, y - mins.y, z - mins.z)) {
			volume.setVoxel(x, y, z, voxel);
		}
	};
	visitVolume(volume, region, 1, 1, 1, visitor, VisitAll());
}

bool fillCheckerboard(voxel::RawVolumeWrapper &volume, const palette::Palette &palette) {
	const int black = palette.getClosestMatch(core::RGBA(0, 0, 0, 255));
	const int white = palette.getClosestMatch(core::RGBA(255, 255, 255, 255));
	if (black == palette::PaletteColorNotFound || white == palette::PaletteColorNotFound) {
		return false;
	}

	const uint8_t colors[2] = {(uint8_t)black, (uint8_t)white};
	int currentColorIndex = 0;

	auto visitor = [&](int x, int y, int z, const voxel::Voxel &) {
		const int idx = colors[currentColorIndex];
		volume.setVoxel(x, y, z, voxel::createVoxel(palette, idx));
		currentColorIndex = (currentColorIndex + 1) % 2;
	};
	visitVolume(volume, visitor, VisitAll());
	return true;
}

void fill(voxel::RawVolumeWrapper &volume, const voxel::Voxel &voxel, bool overwrite) {
	if (overwrite) {
		volume.fill(voxel);
		return;
	}
	auto visitor = [&](int x, int y, int z, const voxel::Voxel &) { volume.setVoxel(x, y, z, voxel); };
	visitVolume(volume, visitor, VisitEmpty());
}

void hollow(voxel::RawVolumeWrapper &volume) {
	core::DynamicArray<glm::ivec3> filled;
	voxelutil::visitUndergroundVolume(
		volume, [&filled](int x, int y, int z, const voxel::Voxel &voxel) { filled.emplace_back(x, y, z); });
	for (const glm::ivec3 &pos : filled) {
		volume.setVoxel(pos, voxel::Voxel());
	}
}

void clear(voxel::RawVolumeWrapper &in) {
	in.clear();
}

using IVec3Set = core::Set<glm::ivec3, 11, glm::hash<glm::ivec3>>;
using WalkCheckCallback = std::function<bool(const voxel::RawVolumeWrapper &, const glm::ivec3 &, voxel::FaceNames)>;
using WalkExecCallback = std::function<bool(voxel::RawVolumeWrapper &, const glm::ivec3 &)>;

template<class CHECK, class EXEC, class Volume>
static int walkPlane_r(IVec3Set &visited, Volume &volume, const voxel::Region &region,
					   CHECK &&check, EXEC &&exec, const glm::ivec3 &position,
					   const glm::ivec3 &offsetForCheckCallback, voxel::FaceNames face) {
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
 * @param checkCallback The callback function to use for checking voxel conditions - if @c true is returned, it's a valid
 * position to check the neighbor for the next steps.
 * @param execCallback The callback function to use for executing operations on the voxels. Only executed if @c check returned
 * @c true
 */
template<class CHECK, class EXEC, class Volume>
static int walkPlane(Volume &volume, glm::ivec3 position, voxel::FaceNames face, int checkOffset,
					 CHECK&& checkCallback, EXEC &&execCallback, int amount) {
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
				  const voxel::Voxel &replaceVoxel) {
	bool firstVoxelIsAir = false;
	bool firstVoxel = true;
	auto check = [&](const voxel::RawVolumeWrapper &in, const glm::ivec3 &p, voxel::FaceNames) {
		return checkOverrideFunc(in, p, replaceVoxel, face, firstVoxelIsAir, firstVoxel);
	};
	auto exec = [=](voxel::RawVolumeWrapper &in, const glm::ivec3 &p) { return in.setVoxel(p, replaceVoxel); };
	return voxelutil::walkPlane(volume, pos, face, -1, check, exec, 1);
}

voxel::Region overridePlaneRegion(const voxel::RawVolume &volume, const glm::ivec3 &pos, voxel::FaceNames face,
								  const voxel::Voxel &replaceVoxel) {
	bool firstVoxelIsAir = false;
	bool firstVoxel = true;
	auto check = [&](const voxel::ModificationRecorder &in, const glm::ivec3 &p, voxel::FaceNames) {
		return checkOverrideFunc(in, p, replaceVoxel, face, firstVoxelIsAir, firstVoxel);
	};
	auto exec = [=](voxel::ModificationRecorder &in, const glm::ivec3 &p) { return in.setVoxel(p, replaceVoxel); };
	voxel::ModificationRecorder recorder(volume);
	voxelutil::walkPlane(recorder, pos, face, -1, check, exec, 1);
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
static bool checkEraseFunc(Volume &volume, const glm::ivec3 &p, const voxel::Voxel &groundVoxel, voxel::FaceNames face) {
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
			   const voxel::Voxel &groundVoxel) {
	auto check = [&](const voxel::RawVolumeWrapper &in, const glm::ivec3 &p, voxel::FaceNames) {
		return checkEraseFunc(in, p, groundVoxel, face);
	};
	auto exec = [](voxel::RawVolumeWrapper &in, const glm::ivec3 &p) {
		return in.setVoxel(p, voxel::Voxel());
	};
	return voxelutil::walkPlane(volume, pos, face, 0, check, exec, 1);
}

voxel::Region erasePlaneRegion(const voxel::RawVolume &volume, const glm::ivec3 &pos, voxel::FaceNames face, const voxel::Voxel &groundVoxel) {
	auto check = [&](const voxel::ModificationRecorder &in, const glm::ivec3 &p, voxel::FaceNames) {
		return checkEraseFunc(in, p, groundVoxel, face);
	};
	auto exec = [](voxel::ModificationRecorder &in, const glm::ivec3 &p) {
		return in.setVoxel(p, voxel::Voxel());
	};
	voxel::ModificationRecorder recorder(volume);
	voxelutil::walkPlane(recorder, pos, face, 0, check, exec, 1);
	return recorder.dirtyRegion();
}

template<class Volume>
static bool checkExtrudeFunc(Volume &volume, const glm::ivec3 &callbackPos,
							 voxel::FaceNames direction, const glm::ivec3 &initialCursorPos,
							 const voxel::Voxel &groundVoxel, const voxel::Voxel &newPlaneVoxel) {
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
	palette::PaletteLookup palLookup;

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
		const core::RGBA rgba = image->colorAt(uv);
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
	voxelutil::visitVolume(
		wrapper, [&wrapper, &newPalette, skipColorIndex, &oldPalette](int x, int y, int z, const voxel::Voxel &voxel) {
			const core::RGBA rgba = oldPalette.color(voxel.getColor());
			const int newColor = newPalette.getClosestMatch(rgba, skipColorIndex);
			if (newColor != palette::PaletteColorNotFound) {
				voxel::Voxel newVoxel(voxel::VoxelType::Generic, newColor, voxel.getNormal(), voxel.getFlags());
				wrapper.setVoxel(x, y, z, newVoxel);
			}
		});
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
