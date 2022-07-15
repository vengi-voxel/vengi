/**
 * @file
 */

#include "VoxelUtil.h"
#include "core/ArrayLength.h"
#include "core/GLM.h"
#include "core/collection/Array3DView.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Set.h"
#include "voxel/Face.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelformat/private/PaletteLookup.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

bool isTouching(const voxel::RawVolume *volume, const glm::ivec3& pos) {
	static const glm::ivec3 offsets[] = {
		glm::ivec3(0, 0, -1),
		glm::ivec3(0, 0, +1),
		glm::ivec3(0, -1, 0),
		glm::ivec3(0, +1, 0),
		glm::ivec3(-1, 0, 0),
		glm::ivec3(+1, 0, 0),
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
		glm::ivec3(+1, +1, 0),
		glm::ivec3(-1, -1, -1),
		glm::ivec3(-1, -1, +1),
		glm::ivec3(-1, +1, -1),
		glm::ivec3(-1, +1, +1),
		glm::ivec3(+1, -1, -1),
		glm::ivec3(+1, -1, +1),
		glm::ivec3(+1, +1, -1),
		glm::ivec3(+1, +1, +1)
	};
	const voxel::Region &region = volume->region();
	for (int i = 0; i < lengthof(offsets); ++i) {
		const glm::ivec3 &volPos = pos + offsets[i];
		if (!region.containsPoint(volPos)) {
			continue;
		}
		if (voxel::isBlocked(volume->voxel(volPos).getMaterial())) {
			return true;
		}
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

static void fillRegion(voxel::RawVolumeWrapper &in, const voxel::Voxel &voxel) {
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
			if (voxel::isAir(in.voxel(v1 + mins).getMaterial())) {
				positions.push_back(v1);
				visited.set(v1, true);
			}
			const glm::ivec3 v2(x, height - 1, z);
			if (voxel::isAir(in.voxel(v2 + mins).getMaterial())) {
				positions.push_back(v2);
				visited.set(v2, true);
			}
		}
		for (int y = 0; y < height; ++y) {
			const glm::ivec3 v1(x, y, 0);
			if (voxel::isAir(in.voxel(v1 + mins).getMaterial())) {
				positions.push_back(v1);
				visited.set(v1, true);
			}
			const glm::ivec3 v2(x, y, depth - 1);
			if (voxel::isAir(in.voxel(v2 + mins).getMaterial())) {
				positions.push_back(v2);
				visited.set(v2, true);
			}
		}
	}
	for (int y = 1; y < height - 1; ++y) {
		for (int z = 1; z < depth - 1; ++z) {
			const glm::ivec3 v1(0, y, z);
			if (voxel::isAir(in.voxel(v1 + mins).getMaterial())) {
				positions.push_back(v1);
				visited.set(v1, true);
			}
			const glm::ivec3 v2(width - 1, y, z);
			if (voxel::isAir(in.voxel(v2 + mins).getMaterial())) {
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

void fillHollow(voxel::RawVolumeWrapper &in, const voxel::Voxel &voxel) {
	fillRegion(in, voxel);
}

using IVec3Set = core::Set<glm::ivec3, 11, glm::hash<glm::ivec3>>;

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
	if (!check(in, checkPosition)) {
		return 0;
	}
	if (!exec(in, position)) {
		return 0;
	}
	int n = 1;
	if (region.containsPoint(position.x + 1, position.y, position.z)) {
		n += walkPlane_r(visited, in, region, check, exec, glm::ivec3(position.x + 1, position.y, position.z), checkOffset,
						 face);
	}
	if (region.containsPoint(position.x - 1, position.y, position.z)) {
		n += walkPlane_r(visited, in, region, check, exec, glm::ivec3(position.x - 1, position.y, position.z), checkOffset,
						 face);
	}
	if (region.containsPoint(position.x, position.y + 1, position.z)) {
		n += walkPlane_r(visited, in, region, check, exec, glm::ivec3(position.x, position.y + 1, position.z), checkOffset,
						 face);
	}
	if (region.containsPoint(position.x, position.y - 1, position.z)) {
		n += walkPlane_r(visited, in, region, check, exec, glm::ivec3(position.x, position.y - 1, position.z), checkOffset,
						 face);
	}
	if (region.containsPoint(position.x, position.y, position.z + 1)) {
		n += walkPlane_r(visited, in, region, check, exec, glm::ivec3(position.x, position.y, position.z + 1), checkOffset,
						 face);
	}
	if (region.containsPoint(position.x, position.y, position.z - 1)) {
		n += walkPlane_r(visited, in, region, check, exec, glm::ivec3(position.x, position.y, position.z - 1), checkOffset,
						 face);
	}
	return n;
}

static int walkPlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &position, voxel::FaceNames face, int checkOffset,
					 const WalkCheckCallback &check, const WalkExecCallback &exec) {
	const voxel::Region &region = in.region();
	glm::ivec3 mins = region.getLowerCorner();
	glm::ivec3 maxs = region.getUpperCorner();
	glm::ivec3 checkOffsetV(0);
	switch (face) {
	case voxel::FaceNames::PositiveX:
		mins.x = position.x;
		maxs.x = position.x;
		if (!region.isOnBorderX(position.x)) {
			checkOffsetV.x = checkOffset;
		}
		break;
	case voxel::FaceNames::NegativeX:
		mins.x = position.x;
		maxs.x = position.x;
		if (!region.isOnBorderX(position.x)) {
			checkOffsetV.x = -checkOffset;
		}
		break;
	case voxel::FaceNames::PositiveY:
		mins.y = position.y;
		maxs.y = position.y;
		if (!region.isOnBorderY(position.y)) {
			checkOffsetV.y = checkOffset;
		}
		break;
	case voxel::FaceNames::NegativeY:
		mins.y = position.y;
		maxs.y = position.y;
		if (!region.isOnBorderY(position.y)) {
			checkOffsetV.y = -checkOffset;
		}
		break;
	case voxel::FaceNames::PositiveZ:
		mins.z = position.z;
		maxs.z = position.z;
		if (!region.isOnBorderZ(position.z)) {
			checkOffsetV.z = checkOffset;
		}
		break;
	case voxel::FaceNames::NegativeZ:
		mins.z = position.z;
		maxs.z = position.z;
		if (!region.isOnBorderZ(position.z)) {
			checkOffsetV.z = -checkOffset;
		}
		break;
	case voxel::FaceNames::Max:
		return -1;
	}
	const voxel::Region walkRegion(mins, maxs);
	const glm::ivec3 &dim = walkRegion.getDimensionsInVoxels();
	const int maxSize = dim.x * dim.y * dim.z;
	IVec3Set visited(maxSize);
	return walkPlane_r(visited, in, walkRegion, check, exec, position, checkOffsetV, face);
}

static glm::vec2 calcUV(const glm::ivec3 &pos, const voxel::Region &region, voxel::FaceNames face) {
	const glm::vec3 dim = region.getDimensionsInVoxels();
	const glm::vec3 r(glm::vec3(pos) / dim);
	switch (face) {
	case voxel::FaceNames::PositiveX:
	case voxel::FaceNames::NegativeX:
		return glm::vec2(r.y, r.z);
	case voxel::FaceNames::PositiveY:
	case voxel::FaceNames::NegativeY:
		return glm::vec2(r.x, r.z);
	case voxel::FaceNames::PositiveZ:
	case voxel::FaceNames::NegativeZ:
		return glm::vec2(r.x, r.y);
	default:
	case voxel::FaceNames::Max:
		return glm::vec2(0.0f);
	}
}

int paintPlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face,
			   const voxel::Voxel &searchVoxel, const voxel::Voxel &replaceVoxel) {
	auto check = [&](const voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) {
		const voxel::Voxel &v = in.voxel(pos);
		return v.getColor() == searchVoxel.getColor();
	};
	auto exec = [=](voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) { return in.setVoxel(pos, replaceVoxel); };
	return voxelutil::walkPlane(in, pos, face, 0, check, exec);
}

int erasePlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face,
			   const voxel::Voxel &groundVoxel) {
	auto check = [&](const voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) {
		const voxel::Voxel &v = in.voxel(pos);
		return v.getColor() == groundVoxel.getColor();
	};
	auto exec = [](voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) { return in.setVoxel(pos, voxel::Voxel()); };
	return voxelutil::walkPlane(in, pos, face, 0, check, exec);
}

int extrudePlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face,
				 const voxel::Voxel &groundVoxel, const voxel::Voxel &newPlaneVoxel) {
	auto check = [&](const voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) {
		const voxel::Voxel &v = in.voxel(pos);
		return v.getColor() == groundVoxel.getColor();
	};
	auto exec = [=](voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) { return in.setVoxel(pos, newPlaneVoxel); };
	return voxelutil::walkPlane(in, pos, face, -1, check, exec);
}

int fillPlane(voxel::RawVolumeWrapper &in, const image::ImagePtr &image, const voxel::Voxel &searchedVoxel,
			  const glm::ivec3 &position, voxel::FaceNames face) {
	voxelformat::PaletteLookup palLookup;

	const voxel::Region &region = in.region();

	auto check = [searchedVoxel](const voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) {
		const voxel::Voxel &v = in.voxel(pos);
		return v == searchedVoxel;
	};

	auto exec = [&](voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) {
		const glm::vec2 &uv = calcUV(pos, region, face);
		const core::RGBA rgba = image->colorAt(uv);
		const uint8_t index = palLookup.findClosestIndex(rgba);
		voxel::Voxel v = voxel::createVoxel(voxel::VoxelType::Generic, index);
		return in.setVoxel(pos, v);
	};

	return walkPlane(in, position, face, 0, check, exec);
}

} // namespace voxelutil
