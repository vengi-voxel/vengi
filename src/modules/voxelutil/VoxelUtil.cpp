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
#include "voxel/Palette.h"
#include "voxel/PaletteLookup.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

bool isTouching(const voxel::RawVolume *volume, const glm::ivec3 &pos) {
	static const glm::ivec3 offsets[] = {
		glm::ivec3(0, 0, -1),	glm::ivec3(0, 0, +1),	glm::ivec3(0, -1, 0),	glm::ivec3(0, +1, 0),
		glm::ivec3(-1, 0, 0),	glm::ivec3(+1, 0, 0),	glm::ivec3(0, -1, -1),	glm::ivec3(0, -1, +1),
		glm::ivec3(0, +1, -1),	glm::ivec3(0, +1, +1),	glm::ivec3(-1, 0, -1),	glm::ivec3(-1, 0, +1),
		glm::ivec3(+1, 0, -1),	glm::ivec3(+1, 0, +1),	glm::ivec3(-1, -1, 0),	glm::ivec3(-1, +1, 0),
		glm::ivec3(+1, -1, 0),	glm::ivec3(+1, +1, 0),	glm::ivec3(-1, -1, -1), glm::ivec3(-1, -1, +1),
		glm::ivec3(-1, +1, -1), glm::ivec3(-1, +1, +1), glm::ivec3(+1, -1, -1), glm::ivec3(+1, -1, +1),
		glm::ivec3(+1, +1, -1), glm::ivec3(+1, +1, +1)};
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

void fillInterpolated(voxel::RawVolume *v, const voxel::Palette &palette) {
	const voxel::Region &region = v->region();
	const glm::ivec3 &mins = region.getLowerCorner();
	const glm::ivec3 &maxs = region.getUpperCorner();
	voxel::RawVolume::Sampler sampler(v);
	for (int x = mins.x; x <= maxs.x; ++x) {
		for (int y = mins.y; y <= maxs.y; ++y) {
			for (int z = mins.z; z <= maxs.z; ++z) {
				sampler.setPosition(x, y, z);
				if (voxel::isBlocked(sampler.voxel().getMaterial())) {
					continue;
				}

				const voxel::Voxel voxel000 = sampler.peekVoxel0px0py0pz();
				const voxel::Voxel voxel001 = sampler.peekVoxel0px0py1pz();
				const voxel::Voxel voxel010 = sampler.peekVoxel0px1py0pz();
				const voxel::Voxel voxel011 = sampler.peekVoxel0px1py1pz();
				const voxel::Voxel voxel100 = sampler.peekVoxel1px0py0pz();
				const voxel::Voxel voxel101 = sampler.peekVoxel1px0py1pz();
				const voxel::Voxel voxel110 = sampler.peekVoxel1px1py0pz();
				const voxel::Voxel voxel111 = sampler.peekVoxel1px1py1pz();

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
				if (blocked < 4) {
					continue;
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
				if (idx == voxel::PaletteColorNotFound) {
					continue;
				}
				sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, idx));
			}
		}
	}
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
	if (!walkRegion.isValid()) {
		return 0;
	}
	const glm::ivec3 &dim = walkRegion.getDimensionsInVoxels();
	const int maxSize = dim.x * dim.y * dim.z;
	core_assert_msg(maxSize > 0, "max size is 0 even though the region was valid");
	IVec3Set visited(maxSize);
	return walkPlane_r(visited, in, walkRegion, check, exec, position, checkOffsetV, face);
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
	auto check = [&](const voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) {
		const voxel::Voxel &v = in.voxel(pos);
		return voxel::isBlocked(v.getMaterial());
	};
	auto exec = [=](voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) { return in.setVoxel(pos, replaceVoxel); };
	return voxelutil::walkPlane(in, pos, face, -1, check, exec);
}

int paintPlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face,
			   const voxel::Voxel &searchVoxel, const voxel::Voxel &replaceVoxel) {
	auto check = [&](const voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) {
		const voxel::Voxel &v = in.voxel(pos);
		return v.isSame(searchVoxel);
	};
	auto exec = [=](voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) { return in.setVoxel(pos, replaceVoxel); };
	return voxelutil::walkPlane(in, pos, face, 0, check, exec);
}

int erasePlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face,
			   const voxel::Voxel &groundVoxel) {
	auto check = [&](const voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) {
		const voxel::Voxel &v = in.voxel(pos);
		return v.isSame(groundVoxel);
	};
	auto exec = [](voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) { return in.setVoxel(pos, voxel::Voxel()); };
	return voxelutil::walkPlane(in, pos, face, 0, check, exec);
}

int extrudePlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face,
				 const voxel::Voxel &groundVoxel, const voxel::Voxel &newPlaneVoxel) {
	auto check = [&](const voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) {
		const voxel::Voxel &v = in.voxel(pos);
		return v.isSame(groundVoxel);
	};
	auto exec = [=](voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) { return in.setVoxel(pos, newPlaneVoxel); };
	return voxelutil::walkPlane(in, pos, face, -1, check, exec);
}

int fillPlane(voxel::RawVolumeWrapper &in, const image::ImagePtr &image, const voxel::Voxel &searchedVoxel,
			  const glm::ivec3 &position, voxel::FaceNames face) {
	voxel::PaletteLookup palLookup;

	const voxel::Region &region = in.region();

	auto check = [searchedVoxel](const voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) {
		if (voxel::isAir(searchedVoxel.getMaterial())) {
			return true;
		}
		const voxel::Voxel &v = in.voxel(pos);
		return v.isSame(searchedVoxel);
	};

	auto exec = [&](voxel::RawVolumeWrapper &in, const glm::ivec3 &pos) {
		const glm::vec2 &uv = calcUV(pos, region, face);
		const core::RGBA rgba = image->colorAt(uv);
		if (rgba.a == 0) {
			return true;
		}
		const uint8_t index = palLookup.findClosestIndex(rgba);
		voxel::Voxel v = voxel::createVoxel(palLookup.palette(), index);
		return in.setVoxel(pos, v);
	};

	return walkPlane(in, position, face, -1, check, exec);
}

voxel::Region remapToPalette(voxel::RawVolume *v, const voxel::Palette &oldPalette, const voxel::Palette &newPalette, int skipColorIndex) {
	if (v == nullptr) {
		return voxel::Region::InvalidRegion;
	}
	voxel::RawVolumeWrapper wrapper(v);
	voxelutil::visitVolume(
		wrapper, [&wrapper, &newPalette, skipColorIndex, &oldPalette](int x, int y, int z, const voxel::Voxel &voxel) {
			const core::RGBA rgba = oldPalette.color(voxel.getColor());
			const int newColor = newPalette.getClosestMatch(rgba, nullptr, skipColorIndex);
			if (newColor != voxel::PaletteColorNotFound) {
				voxel::Voxel newVoxel(voxel::VoxelType::Generic, newColor);
				wrapper.setVoxel(x, y, z, newVoxel);
			}
		});
	return wrapper.dirtyRegion();
}

} // namespace voxelutil
