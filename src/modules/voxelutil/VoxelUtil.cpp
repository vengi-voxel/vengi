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
#include "math/Axis.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include <functional>

namespace voxelutil {

bool isTouching(const voxel::RawVolume &volume, const glm::ivec3 &pos, Connectivity connectivity) {
	static const glm::ivec3 arrayPathfinderFaces[6] = {
			glm::ivec3(0, 0, -1),
			glm::ivec3(0, 0, +1),
			glm::ivec3(0, -1, 0),
			glm::ivec3(0, +1, 0),
			glm::ivec3(-1, 0, 0),
			glm::ivec3(+1, 0, 0) };

	static const glm::ivec3 arrayPathfinderEdges[12] = {
			glm::ivec3(0, -1, -1),
			glm::ivec3(0, -1, +1),
			glm::ivec3(0, +1, -1),
			glm::ivec3(0, +1, +1),
			glm::ivec3(-1, 0, -1),
			glm::ivec3(-1, 0, +1),
			glm::ivec3(+1, 0, -1),
			glm::ivec3(+1, 0, +1),
			glm::ivec3(-1, -1, 0),
			glm::ivec3(-1, +1, 0),
			glm::ivec3(+1, -1, 0),
			glm::ivec3(+1, +1, 0) };

	static const glm::ivec3 arrayPathfinderCorners[8] = {
			glm::ivec3(-1, -1, -1),
			glm::ivec3(-1, -1, +1),
			glm::ivec3(-1, +1, -1),
			glm::ivec3(-1, +1, +1),
			glm::ivec3(+1, -1, -1),
			glm::ivec3(+1, -1, +1),
			glm::ivec3(+1, +1, -1),
			glm::ivec3(+1, +1, +1) };

	voxel::RawVolume::Sampler sampler(volume);
	if (!sampler.setPosition(pos)) {
		return false;
	}
	switch (connectivity) {
	case Connectivity::TwentySixConnected:
		for (const glm::ivec3 &offset : arrayPathfinderCorners) {
			const glm::ivec3 &volPos = pos + offset;
			if (!sampler.setPosition(volPos)) {
				continue;
			}
			if (voxel::isBlocked(sampler.voxel().getMaterial())) {
				return true;
			}
		}
		/* fallthrough */

	case Connectivity::EighteenConnected:
		for (const glm::ivec3 &offset : arrayPathfinderEdges) {
			const glm::ivec3 &volPos = pos + offset;
			if (!sampler.setPosition(volPos)) {
				continue;
			}
			if (voxel::isBlocked(sampler.voxel().getMaterial())) {
				return true;
			}
		}
		/* fallthrough */

	case Connectivity::SixConnected:
		for (const glm::ivec3 &offset : arrayPathfinderFaces) {
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

voxel::Voxel getInterpolated(voxel::RawVolumeWrapper &v, const glm::ivec3 &pos, const palette::Palette &palette) {
	voxel::RawVolumeWrapper::Sampler sampler3(v);
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

bool copy(const voxel::RawVolume &in, const voxel::Region &inRegion, voxel::RawVolume &out,
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
				const voxel::Voxel &voxel = in.voxel(xIn, yIn, zIn);
				wrapper.setVoxel(xOut, yOut, zOut, voxel);
			}
		}
	}
	return wrapper.dirtyRegion().isValid();
}

bool copyIntoRegion(const voxel::RawVolume &in, voxel::RawVolume &out, const voxel::Region &targetRegion) {
	return copy(in, in.region(), out, targetRegion);
}

void fillHollow(voxel::RawVolumeWrapper &in, const voxel::Voxel &voxel) {
	const voxel::Region &region = in.region();
	const int width = region.getWidthInVoxels();
	const int height = region.getHeightInVoxels();
	const int depth = region.getDepthInVoxels();
	const int size = width * height * depth;
	const glm::ivec3 mins = in.region().getLowerCorner();
	core::DynamicArray<glm::ivec3> positions;
	positions.reserve(size);
	core::Buffer<bool> visitedData(size);
	core::Array3DView<bool> visited(visitedData.data(), width, height, depth);

	visitVolume(
		in, region, 1, 1, 1,
		[&](int x, int y, int z, const voxel::Voxel &) { visited.set(x - mins.x, y - mins.y, z - mins.z, true); },
		SkipEmpty());

	for (int x = 0; x < width; ++x) {
		for (int z = 1; z < depth - 1; ++z) {
			const glm::ivec3 v1(x, 0, z);
			const voxel::VoxelType m1 = in.voxel(v1 + mins).getMaterial();
			if (voxel::isAir(m1) || voxel::isTransparent(m1)) {
				positions.push_back(v1);
				visited.set(v1, true);
			}
			const glm::ivec3 v2(x, height - 1, z);
			const voxel::VoxelType m2 = in.voxel(v2 + mins).getMaterial();
			if (voxel::isAir(m2) || voxel::isTransparent(m2)) {
				positions.push_back(v2);
				visited.set(v2, true);
			}
		}
		for (int y = 0; y < height; ++y) {
			const glm::ivec3 v1(x, y, 0);
			const voxel::VoxelType m1 = in.voxel(v1 + mins).getMaterial();
			if (voxel::isAir(m1) || voxel::isTransparent(m1)) {
				positions.push_back(v1);
				visited.set(v1, true);
			}
			const glm::ivec3 v2(x, y, depth - 1);
			const voxel::VoxelType m2 = in.voxel(v2 + mins).getMaterial();
			if (voxel::isAir(m2) || voxel::isTransparent(m2)) {
				positions.push_back(v2);
				visited.set(v2, true);
			}
		}
	}
	for (int y = 1; y < height - 1; ++y) {
		for (int z = 1; z < depth - 1; ++z) {
			const glm::ivec3 v1(0, y, z);
			const voxel::VoxelType m1 = in.voxel(v1 + mins).getMaterial();
			if (voxel::isAir(m1) || voxel::isTransparent(m1)) {
				positions.push_back(v1);
				visited.set(v1, true);
			}
			const glm::ivec3 v2(width - 1, y, z);
			const voxel::VoxelType m2 = in.voxel(v2 + mins).getMaterial();
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

	visitVolume(
		in, region, 1, 1, 1,
		[&](int x, int y, int z, const voxel::Voxel &) {
			if (!visited.get(x - mins.x, y - mins.y, z - mins.z)) {
				in.setVoxel(x, y, z, voxel);
			}
		},
		VisitAll());
}

void fill(voxel::RawVolumeWrapper &in, const voxel::Voxel &voxel, bool overwrite) {
	if (overwrite) {
		visitVolume(
			in, [&](int x, int y, int z, const voxel::Voxel &) { in.setVoxel(x, y, z, voxel); }, VisitAll());
		return;
	}
	visitVolume(
		in,
		[&](int x, int y, int z, const voxel::Voxel &v) {
			if (voxel::isAir(v.getMaterial())) {
				in.setVoxel(x, y, z, voxel);
			}
		},
		VisitAll());
}

void hollow(voxel::RawVolumeWrapper &in) {
	core::DynamicArray<glm::ivec3> filled;
	voxelutil::visitUndergroundVolume(
		in, [&filled](int x, int y, int z, const voxel::Voxel &voxel) { filled.emplace_back(x, y, z); });
	for (const glm::ivec3 &pos : filled) {
		in.setVoxel(pos, voxel::Voxel());
	}
}

void clear(voxel::RawVolumeWrapper &in) {
	in.clear();
}

using IVec3Set = core::Set<glm::ivec3, 11, glm::hash<glm::ivec3>>;
using WalkCheckCallback = std::function<bool(const voxel::RawVolumeWrapper &, const glm::ivec3 &, voxel::FaceNames)>;
using WalkExecCallback = std::function<bool(voxel::RawVolumeWrapper &, const glm::ivec3 &)>;

static int walkPlane_r(IVec3Set &visited, voxel::RawVolumeWrapper &in, const voxel::Region &region,
					   const WalkCheckCallback &check, const WalkExecCallback &exec, const glm::ivec3 &position,
					   const glm::ivec3 &checkOffset, voxel::FaceNames face) {
	const glm::ivec3 checkPosition = position + checkOffset;
	if (visited.has(checkPosition)) {
		return 0;
	}
	visited.insert(checkPosition);
	if (!region.containsPoint(position)) {
		return 0;
	}
	if (!check(in, checkPosition, face)) {
		return 0;
	}
	if (!exec(in, position)) {
		return 0;
	}
	int n = 1;
	if (region.containsPoint(position.x + 1, position.y, position.z)) {
		n += walkPlane_r(visited, in, region, check, exec, glm::ivec3(position.x + 1, position.y, position.z),
						 checkOffset, face);
	}
	if (region.containsPoint(position.x - 1, position.y, position.z)) {
		n += walkPlane_r(visited, in, region, check, exec, glm::ivec3(position.x - 1, position.y, position.z),
						 checkOffset, face);
	}
	if (region.containsPoint(position.x, position.y + 1, position.z)) {
		n += walkPlane_r(visited, in, region, check, exec, glm::ivec3(position.x, position.y + 1, position.z),
						 checkOffset, face);
	}
	if (region.containsPoint(position.x, position.y - 1, position.z)) {
		n += walkPlane_r(visited, in, region, check, exec, glm::ivec3(position.x, position.y - 1, position.z),
						 checkOffset, face);
	}
	if (region.containsPoint(position.x, position.y, position.z + 1)) {
		n += walkPlane_r(visited, in, region, check, exec, glm::ivec3(position.x, position.y, position.z + 1),
						 checkOffset, face);
	}
	if (region.containsPoint(position.x, position.y, position.z - 1)) {
		n += walkPlane_r(visited, in, region, check, exec, glm::ivec3(position.x, position.y, position.z - 1),
						 checkOffset, face);
	}
	return n;
}

/**
 * @brief Walks a plane in a voxel volume based on the given position and face direction.
 *
 * @param in The voxel volume to walk the plane in.
 * @param position The position in the voxel volume to start the walk from.
 * @param face The direction of the face to walk the plane in.
 * @param checkOffset The offset for checking voxel positions.
 * @param check The callback function to use for checking voxel conditions - if @c true is returned, it's a valid
 * position to check the neighbor for the next steps.
 * @param exec The callback function to use for executing operations on the voxels. Only executed if @c check returned
 * @c true
 */
static int walkPlane(voxel::RawVolumeWrapper &in, glm::ivec3 position, voxel::FaceNames face, int checkOffset,
					 const WalkCheckCallback &check, const WalkExecCallback &exec, int amount) {
	const voxel::Region &region = in.region();
	glm::ivec3 mins = region.getLowerCorner();
	glm::ivec3 maxs = region.getUpperCorner();
	glm::ivec3 checkOffsetV(0);
	switch (face) {
	case voxel::FaceNames::PositiveX:
		mins.x = position.x;
		maxs.x = position.x;
		checkOffsetV.x = checkOffset;
		break;
	case voxel::FaceNames::NegativeX:
		mins.x = position.x;
		maxs.x = position.x;
		checkOffsetV.x = -checkOffset;
		break;
	case voxel::FaceNames::PositiveY:
		mins.y = position.y;
		maxs.y = position.y;
		checkOffsetV.y = checkOffset;
		break;
	case voxel::FaceNames::NegativeY:
		mins.y = position.y;
		maxs.y = position.y;
		checkOffsetV.y = -checkOffset;
		break;
	case voxel::FaceNames::PositiveZ:
		mins.z = position.z;
		maxs.z = position.z;
		checkOffsetV.z = checkOffset;
		break;
	case voxel::FaceNames::NegativeZ:
		mins.z = position.z;
		maxs.z = position.z;
		checkOffsetV.z = -checkOffset;
		break;
	case voxel::FaceNames::Max:
		return -1;
	}

	const math::Axis axis = voxel::faceToAxis(face);
	const int idx = math::getIndexForAxis(axis);
	const int offset = voxel::isNegativeFace(face) ? -1 : 1;

	int n = 0;
	for (int i = 0; i < amount; ++i) {
		const voxel::Region walkRegion(mins, maxs);
		if (!walkRegion.isValid()) {
			return 0;
		}
		const glm::ivec3 &dim = walkRegion.getDimensionsInVoxels();
		const int maxSize = dim.x * dim.y * dim.z;
		core_assert_msg(maxSize > 0, "max size is 0 even though the region was valid");
		IVec3Set visited(maxSize);
		const int n0 = walkPlane_r(visited, in, walkRegion, check, exec, position, checkOffsetV, face);
		if (n0 == 0) {
			break;
		}
		mins[idx] += offset;
		maxs[idx] += offset;
		position[idx] += offset;
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

int overridePlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face,
				  const voxel::Voxel &replaceVoxel) {
	bool firstVoxelIsAir = false;
	bool firstVoxel = true;
	auto check = [&](const voxel::RawVolumeWrapper &volume, const glm::ivec3 &p, voxel::FaceNames) {
		const voxel::Voxel &v = volume.voxel(p);
		if (firstVoxel) {
			firstVoxelIsAir = voxel::isAir(v.getMaterial());
		}
		firstVoxel = false;
		if (firstVoxelIsAir) {
			return voxel::isAir(v.getMaterial());
		}
		return voxel::isBlocked(v.getMaterial());
	};
	auto exec = [=](voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) {
		return in.setVoxel(pos, replaceVoxel);
	};
	return voxelutil::walkPlane(in, pos, face, -1, check, exec, 1);
}

int paintPlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face,
			   const voxel::Voxel &searchVoxel, const voxel::Voxel &replaceVoxel) {
	auto check = [&](const voxel::RawVolumeWrapper &volume, const glm::ivec3 &p, voxel::FaceNames) {
		const voxel::Voxel &v = volume.voxel(p);
		return v.isSame(searchVoxel);
	};
	auto exec = [=](voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) {
		return in.setVoxel(pos, replaceVoxel);
	};
	return voxelutil::walkPlane(in, pos, face, 0, check, exec, 1);
}

int erasePlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face,
			   const voxel::Voxel &groundVoxel) {
	auto check = [&](const voxel::RawVolumeWrapper &volume, const glm::ivec3 &p, voxel::FaceNames) {
		const voxel::Voxel &v = volume.voxel(p);
		if (v.isSame(groundVoxel)) {
			const math::Axis axis = voxel::faceToAxis(face);
			const int idx = math::getIndexForAxis(axis);
			const int offset = voxel::isNegativeFace(face) ? -1 : 1;
			glm::ivec3 abovePos = p;
			abovePos[idx] += offset;
			const voxel::Voxel &aboveVoxel = volume.voxel(abovePos);
			return voxel::isAir(aboveVoxel.getMaterial());
		}
		return false;
	};
	auto exec = [](voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) {
		return in.setVoxel(pos, voxel::Voxel());
	};
	return voxelutil::walkPlane(in, pos, face, 0, check, exec, 1);
}

int extrudePlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face,
				 const voxel::Voxel &groundVoxel, const voxel::Voxel &newPlaneVoxel, int thickness) {
	auto check = [&](const voxel::RawVolumeWrapper &volume, const glm::ivec3 &p, voxel::FaceNames direction) {
		const voxel::Voxel &v = volume.voxel(p);
		const math::Axis axis = voxel::faceToAxis(direction);
		const int idx = math::getIndexForAxis(axis);
		const int offset = voxel::isNegativeFace(direction) ? -1 : 1;
		if (p[idx] + offset == pos[idx]) {
			return v.isSame(groundVoxel);
		}
		return v.isSame(newPlaneVoxel);
	};
	auto exec = [=](voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) {
		return in.setVoxel(pos, newPlaneVoxel);
	};
	return voxelutil::walkPlane(in, pos, face, -1, check, exec, thickness);
}

int fillPlane(voxel::RawVolumeWrapper &in, const image::ImagePtr &image, const voxel::Voxel &searchedVoxel,
			  const glm::ivec3 &position, voxel::FaceNames face) {
	palette::PaletteLookup palLookup;

	const voxel::Region &region = in.region();

	auto check = [searchedVoxel](const voxel::RawVolumeWrapper &volume, const glm::ivec3 &p, voxel::FaceNames) {
		if (voxel::isAir(searchedVoxel.getMaterial())) {
			return true;
		}
		const voxel::Voxel &v = volume.voxel(p);
		return v.isSame(searchedVoxel);
	};

	auto exec = [&](voxel::RawVolumeWrapper &volume, const glm::ivec3 &p) {
		const glm::vec2 &uv = calcUV(p, region, face);
		const core::RGBA rgba = image->colorAt(uv);
		if (rgba.a == 0) {
			return true;
		}
		const uint8_t index = palLookup.findClosestIndex(rgba);
		voxel::Voxel v = voxel::createVoxel(palLookup.palette(), index);
		return volume.setVoxel(p, v);
	};

	return walkPlane(in, position, face, -1, check, exec, 1);
}

voxel::Region remapToPalette(voxel::RawVolume *v, const palette::Palette &oldPalette,
							 const palette::Palette &newPalette, int skipColorIndex) {
	if (v == nullptr) {
		return voxel::Region::InvalidRegion;
	}
	voxel::RawVolumeWrapper wrapper(v);
	voxelutil::visitVolume(
		wrapper, [&wrapper, &newPalette, skipColorIndex, &oldPalette](int x, int y, int z, const voxel::Voxel &voxel) {
			const core::RGBA rgba = oldPalette.color(voxel.getColor());
			const int newColor = newPalette.getClosestMatch(rgba, skipColorIndex);
			if (newColor != palette::PaletteColorNotFound) {
				voxel::Voxel newVoxel(voxel::VoxelType::Generic, newColor);
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
