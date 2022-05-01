/**
 * @file
 */

#include "VoxelUtil.h"
#include "core/collection/Array3DView.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "voxel/Face.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

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

static void fillRegion(voxel::RawVolume &in, const voxel::Voxel &voxel, const voxel::Region &region) {
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

void fillHollow(voxel::RawVolume &in, const voxel::Voxel &voxel) {
	fillRegion(in, voxel, in.region());
}

static void fillPlane_r(voxel::RawVolumeWrapper &in, const voxel::Region &region, const voxel::Voxel &voxel,
						const voxel::Voxel &replace, const glm::ivec3 &position) {
	if (!region.containsPoint(position)) {
		return;
	}
	if (in.voxel(position).getColor() != replace.getColor()) {
		return;
	}
	if (!in.setVoxel(position, voxel)) {
		return;
	}
	if (region.containsPoint(position.x + 1, position.y, position.z)) {
		fillPlane_r(in, region, voxel, replace, glm::ivec3(position.x + 1, position.y, position.z));
	}
	if (region.containsPoint(position.x - 1, position.y, position.z)) {
		fillPlane_r(in, region, voxel, replace, glm::ivec3(position.x - 1, position.y, position.z));
	}
	if (region.containsPoint(position.x, position.y + 1, position.z)) {
		fillPlane_r(in, region, voxel, replace, glm::ivec3(position.x, position.y + 1, position.z));
	}
	if (region.containsPoint(position.x, position.y - 1, position.z)) {
		fillPlane_r(in, region, voxel, replace, glm::ivec3(position.x, position.y - 1, position.z));
	}
	if (region.containsPoint(position.x, position.y, position.z + 1)) {
		fillPlane_r(in, region, voxel, replace, glm::ivec3(position.x, position.y, position.z + 1));
	}
	if (region.containsPoint(position.x, position.y, position.z - 1)) {
		fillPlane_r(in, region, voxel, replace, glm::ivec3(position.x, position.y, position.z - 1));
	}
}

void fillPlane(voxel::RawVolumeWrapper &in, const voxel::Voxel &voxel, const voxel::Voxel &replace, const glm::ivec3 &position,
			   voxel::FaceNames face) {
	if (voxel.getColor() == replace.getColor()) {
		return;
	}
	glm::ivec3 mins = in.region().getLowerCorner();
	glm::ivec3 maxs = in.region().getUpperCorner();
	switch (face) {
	case voxel::FaceNames::PositiveX:
	case voxel::FaceNames::NegativeX:
		mins.x = position.x;
		maxs.x = position.x;
		break;
	case voxel::FaceNames::PositiveY:
	case voxel::FaceNames::NegativeY:
		mins.y = position.y;
		maxs.y = position.y;
		break;
	case voxel::FaceNames::PositiveZ:
	case voxel::FaceNames::NegativeZ:
		mins.z = position.z;
		maxs.z = position.z;
		break;
	case voxel::FaceNames::Max:
		return;
	}
	const voxel::Region region(mins, maxs);
	fillPlane_r(in, region, voxel, replace, position);
}

} // namespace voxelutil
