/**
 * @file
 */

#include "ExtrudeOperation.h"
#include "app/App.h"
#include "app/ForParallel.h"
#include "core/Common.h"
#include "math/Axis.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Connectivity.h"
#include "voxel/DynamicVoxelArray.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include <glm/gtc/matrix_transform.hpp>

namespace voxedit {

void ExtrudeOperation::cacheSelection(const voxel::RawVolume *vol, const voxel::Region &volRegion) {
	_cachedSelectedPositions.clear();
	_cachedWallCandidates.clear();
	_hasCachedSelection = false;

	if (_face == voxel::FaceNames::Max) {
		return;
	}

	// Collect selected (FlagOutline) voxel positions using parallel scan.
	// Each thread collects into its own array - no locks needed.
	const int zStart = volRegion.getLowerZ();
	const int zEnd = volRegion.getUpperZ() + 1;
	const int zRange = zEnd - zStart;
	const int threadCount = app::App::getInstance()->threads();
	const int chunkCount = (threadCount > 1 && zRange > 1) ? core_min(threadCount, zRange) : 1;

	core::DynamicArray<core::DynamicArray<glm::ivec3>> threadResults;
	threadResults.resize(chunkCount);

	app::for_parallel(0, chunkCount, [&](int chunkStart, int chunkEnd) {
		for (int chunk = chunkStart; chunk < chunkEnd; ++chunk) {
			const int sliceStart = zStart + (zRange * chunk) / chunkCount;
			const int sliceEnd = zStart + (zRange * (chunk + 1)) / chunkCount;
			const voxel::Region subRegion(
				volRegion.getLowerX(), volRegion.getLowerY(), sliceStart,
				volRegion.getUpperX(), volRegion.getUpperY(), sliceEnd - 1);
			voxelutil::visitVolume(*vol, subRegion,
				[&threadResults, chunk](int x, int y, int z, const voxel::Voxel &) {
					threadResults[chunk].push_back(glm::ivec3(x, y, z));
				},
				voxelutil::VisitSolidOutline());
		}
	});

	// Merge results
	glm::ivec3 selLo(volRegion.getUpperCorner());
	glm::ivec3 selHi(volRegion.getLowerCorner());
	PositionSet selectedSet;
	for (auto &arr : threadResults) {
		for (const glm::ivec3 &pos : arr) {
			_cachedSelectedPositions.push_back(pos);
			selectedSet.insert(pos);
			selLo = glm::min(selLo, pos);
			selHi = glm::max(selHi, pos);
		}
	}

	if (_cachedSelectedPositions.empty()) {
		return;
	}

	_cachedSelectedBBox = voxel::Region(selLo, selHi);

	// Compute wall candidates: for each selected voxel, check 4 perpendicular neighbors.
	// A wall candidate is a (selectedPos, perpOffset) pair where the perpendicular neighbor
	// is solid and not selected - indicating an edge that needs sealing during extrusion.
	const math::Axis axis = voxel::faceToAxis(_face);
	const int axisIdx = math::getIndexForAxis(axis);
	static constexpr int NumAxes = 3;
	const int perp1 = (axisIdx + 1) % NumAxes;
	const int perp2 = (axisIdx + 2) % NumAxes;

	static constexpr int NumPerpOffsets = 4;
	glm::ivec3 perpOffsets[NumPerpOffsets] = {};
	perpOffsets[0][perp1] = 1;
	perpOffsets[1][perp1] = -1;
	perpOffsets[2][perp2] = 1;
	perpOffsets[3][perp2] = -1;

	for (const glm::ivec3 &selPos : _cachedSelectedPositions) {
		for (int pi = 0; pi < NumPerpOffsets; ++pi) {
			const glm::ivec3 neighborPos = selPos + perpOffsets[pi];
			if (!volRegion.containsPoint(neighborPos)) {
				continue;
			}
			const voxel::Voxel &nv = vol->voxel(neighborPos);
			if (voxel::isAir(nv.getMaterial())) {
				continue;
			}
			if (selectedSet.has(neighborPos)) {
				continue;
			}
			_cachedWallCandidates.push_back({selPos, perpOffsets[pi]});
		}
	}

	_capturedVolumeLower = volRegion.getLowerCorner();
	_hasCachedSelection = true;
}

void ExtrudeOperation::adjustCacheForRegionShift(const glm::ivec3 &delta) {
	for (glm::ivec3 &pos : _cachedSelectedPositions) {
		pos += delta;
	}
	_cachedSelectedBBox.shift(delta.x, delta.y, delta.z);

	for (WallCandidate &wc : _cachedWallCandidates) {
		wc.basePos += delta;
	}

	for (voxel::VoxelPosition &entry : _history) {
		entry.pos += delta;
	}

	_capturedVolumeLower += delta;
}

void ExtrudeOperation::writeVoxel(ModifierVolumeWrapper &wrapper, PositionSet &savedPositions, const glm::ivec3 &pos, const voxel::Voxel &newVoxel) {
	voxel::RawVolume *vol = wrapper.volume();
	const voxel::Region &volRegion = vol->region();
	if (!volRegion.containsPoint(pos)) {
		return;
	}
	if (!savedPositions.has(pos)) {
		_history.push_back({pos, vol->voxel(pos)});
		savedPositions.insert(pos);
	}
	vol->setVoxel(pos, newVoxel);
	wrapper.addToDirtyRegion(pos);
}

void ExtrudeOperation::evaluate(ModifierVolumeWrapper &wrapper, const voxel::Voxel &voxel) {
	voxel::RawVolume *vol = wrapper.volume();

	// Step 1: Restore all positions modified by the previous generate() call.
	// This makes depth/offset changes fully reversible without touching the undo stack.
	// Track the bounding box of all restored positions for dirty region.
	const voxel::Region &volRegion = vol->region();
	glm::ivec3 dirtyLo(volRegion.getUpperCorner());
	glm::ivec3 dirtyHi(volRegion.getLowerCorner());
	for (const voxel::VoxelPosition &entry : _history) {
		vol->setVoxel(entry.pos, entry.voxel);
		dirtyLo = glm::min(dirtyLo, entry.pos);
		dirtyHi = glm::max(dirtyHi, entry.pos);
	}
	if (!_history.empty()) {
		wrapper.addToDirtyRegion(dirtyLo);
		wrapper.addToDirtyRegion(dirtyHi);
	}
	_history.clear();

	if (_depth == 0 || _face == voxel::FaceNames::Max || !_hasCachedSelection) {
		return;
	}

	const math::Axis axis = voxel::faceToAxis(_face);
	const int axisIdx = math::getIndexForAxis(axis);
	const int faceSign = voxel::isNegativeFace(_face) ? -1 : 1;

	const bool carving = _depth < 0;
	const int steps = glm::abs(_depth);
	const int dirSign = carving ? -faceSign : faceSign;

	glm::ivec3 dir(0);
	dir[axisIdx] = dirSign;

	static constexpr int NumAxes = 3;
	const int perp1 = (axisIdx + 1) % NumAxes;
	const int perp2 = (axisIdx + 2) % NumAxes;

	const float stepU = static_cast<float>(_offsetU) / steps;
	const float stepV = static_cast<float>(_offsetV) / steps;
	auto lateralShift = [&](int step) -> glm::ivec3 {
		glm::ivec3 shift(0);
		if (_offsetU != 0) {
			shift[perp1] = static_cast<int>(stepU * step);
		}
		if (_offsetV != 0) {
			shift[perp2] = static_cast<int>(stepV * step);
		}
		return shift;
	};

	// Step 2: Save original voxel before writing - uses a set for O(1) dedup.
	// Saves both air and solid voxels so restore is always complete.
	PositionSet savedPositions;

	const voxel::Voxel air{};

	// Step 3: Apply extrusion or carving
	if (carving) {
		// Loop 1: Place walls at each depth step before carving
		if (_fillWalls) {
			for (int step = 0; step < steps; ++step) {
				const glm::ivec3 shift = lateralShift(step);
				for (const WallCandidate &wc : _cachedWallCandidates) {
					const glm::ivec3 sidePos = wc.basePos + wc.perpOffset + dir * step + shift;
					writeVoxel(wrapper, savedPositions, sidePos, voxel);
				}
			}
		}
		// Loop 2: Carve out cached selected positions at each depth step
		for (int step = 0; step < steps; ++step) {
			const glm::ivec3 shift = lateralShift(step);
			for (const glm::ivec3 &selPos : _cachedSelectedPositions) {
				writeVoxel(wrapper, savedPositions, selPos + dir * step + shift, air);
			}
		}
	} else {
		// Positive depth: place selected voxels at each step from 1 to depth
		for (int step = 1; step <= steps; ++step) {
			const glm::ivec3 shift = lateralShift(step);
			for (const glm::ivec3 &selPos : _cachedSelectedPositions) {
				writeVoxel(wrapper, savedPositions, selPos + dir * step + shift, voxel);
			}
		}
		// Include the original selected positions in the dirty region so that
		// auto-select will re-select them along with the newly extruded voxels
		wrapper.addToDirtyRegion(_cachedSelectedPositions);
	}
}

} // namespace voxedit
