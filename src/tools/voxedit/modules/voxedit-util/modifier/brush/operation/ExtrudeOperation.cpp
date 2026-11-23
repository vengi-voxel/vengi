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
#include "math/Vec3i.h"

namespace voxedit {

void ExtrudeOperation::prepare(const voxel::RawVolume *volume, const voxel::Region &region) {
	if (volume && !_hasCachedSelection && _parameters.face != voxel::FaceNames::Max) {
		cacheSelection(volume, region);
	}
	_cachedSelBBox = _cachedSelectedBBox;
	_cachedSelBBoxValid = _hasCachedSelection;
}

bool ExtrudeOperation::setParameters(const Parameters &parameters) {
	if (parameters.face > voxel::FaceNames::Max) {
		return false;
	}
	if (_parameters.face != parameters.face) {
		_hasCachedSelection = false;
	}
	_parameters = parameters;
	return true;
}

void ExtrudeOperation::prune(ModifierVolumeWrapper &wrapper, PositionSet &saved) {
	// Prune interior voxels after final commit - voxels fully enclosed by 6 solid
	// neighbors are invisible and waste sparse storage.
	if (wrapper.volume() && _parameters.depth > 0) {
		const voxel::Region &volRegion = wrapper.volume()->region();
		const voxel::Voxel air;
		auto isInterior = [&](const math::Vec3i &pos) {
			for (int ni = 0; ni < lengthof(voxel::arrayPathfinderFaces); ++ni) {
				const math::Vec3i nb = pos + voxel::arrayPathfinderFaces[ni];
				if (!volRegion.containsPoint(nb) || voxel::isAir(wrapper.volume()->voxel(nb).getMaterial())) {
					return false;
				}
			}
			return true;
		};

		core::DynamicArray<math::Vec3i> toPrune;
		toPrune.reserve(_history.size() + _cachedSelectedPositions.size());
		for (const voxel::VoxelPosition &entry : _history) {
			if (!voxel::isAir(wrapper.volume()->voxel(entry.pos).getMaterial()) && isInterior(entry.pos)) {
				toPrune.push_back(entry.pos);
			}
		}
		for (const math::Vec3i &selPos : _cachedSelectedPositions) {
			if (!voxel::isAir(wrapper.volume()->voxel(selPos).getMaterial()) && isInterior(selPos)) {
				toPrune.push_back(selPos);
			}
		}
		for (const math::Vec3i &pos : toPrune) {
			writeVoxel(wrapper, saved, pos, air);
		}
	}
}

void ExtrudeOperation::cacheSelection(const voxel::RawVolume *vol, const voxel::Region &volRegion) {
	_cachedSelectedPositions.clear();
	_cachedWallCandidates.clear();
	_hasCachedSelection = false;

	if (_parameters.face == voxel::FaceNames::Max) {
		return;
	}

	// Collect selected (FlagOutline) voxel positions using parallel scan.
	// Each thread collects into its own array - no locks needed.
	const int zStart = volRegion.getLowerZ();
	const int zEnd = volRegion.getUpperZ() + 1;
	const int zRange = zEnd - zStart;
	const int threadCount = app::App::getInstance()->threads();
	const int chunkCount = (threadCount > 1 && zRange > 1) ? core_min(threadCount, zRange) : 1;

	core::DynamicArray<core::DynamicArray<math::Vec3i>> threadResults;
	threadResults.resize(chunkCount);

	app::for_parallel(0, chunkCount, [&](int chunkStart, int chunkEnd) {
		for (int chunk = chunkStart; chunk < chunkEnd; ++chunk) {
			const int sliceStart = zStart + (zRange * chunk) / chunkCount;
			const int sliceEnd = zStart + (zRange * (chunk + 1)) / chunkCount;
			const voxel::Region subRegion(volRegion.getLowerX(), volRegion.getLowerY(), sliceStart,
										  volRegion.getUpperX(), volRegion.getUpperY(), sliceEnd - 1);
			voxelutil::visitVolume(
				*vol, subRegion,
				[&threadResults, chunk](int x, int y, int z, const voxel::Voxel &) {
					threadResults[chunk].push_back(math::Vec3i(x, y, z));
				},
				voxelutil::VisitSolidOutline());
		}
	});

	// Merge results
	math::Vec3i selLo(volRegion.getUpperCorner());
	math::Vec3i selHi(volRegion.getLowerCorner());
	PositionSet selectedSet;
	for (auto &arr : threadResults) {
		for (const math::Vec3i &pos : arr) {
			_cachedSelectedPositions.push_back(pos);
			selectedSet.insert(pos);
			selLo = math::min(selLo, pos);
			selHi = math::max(selHi, pos);
		}
	}

	if (_cachedSelectedPositions.empty()) {
		return;
	}

	_cachedSelectedBBox = voxel::Region(selLo, selHi);

	// Compute wall candidates: for each selected voxel, check 4 perpendicular neighbors.
	// A wall candidate is a (selectedPos, perpOffset) pair where the perpendicular neighbor
	// is solid and not selected - indicating an edge that needs sealing during extrusion.
	const math::Axis axis = voxel::faceToAxis(_parameters.face);
	const int axisIdx = math::getIndexForAxis(axis);
	static constexpr int NumAxes = 3;
	const int perp1 = (axisIdx + 1) % NumAxes;
	const int perp2 = (axisIdx + 2) % NumAxes;

	static constexpr int NumPerpOffsets = 4;
	math::Vec3i perpOffsets[NumPerpOffsets] = {};
	perpOffsets[0][perp1] = 1;
	perpOffsets[1][perp1] = -1;
	perpOffsets[2][perp2] = 1;
	perpOffsets[3][perp2] = -1;

	for (const math::Vec3i &selPos : _cachedSelectedPositions) {
		for (int pi = 0; pi < NumPerpOffsets; ++pi) {
			const math::Vec3i neighborPos = selPos + perpOffsets[pi];
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

void ExtrudeOperation::adjustCacheForRegionShift(const math::Vec3i &delta) {
	for (math::Vec3i &pos : _cachedSelectedPositions) {
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

void ExtrudeOperation::writeVoxel(ModifierVolumeWrapper &wrapper, PositionSet &savedPositions, const math::Vec3i &pos,
								  const voxel::Voxel &newVoxel) {
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

void ExtrudeOperation::evaluate(ModifierVolumeWrapper &wrapper) {
	voxel::RawVolume *vol = wrapper.volume();

	// Step 1: Restore all positions modified by the previous evaluate() call.
	// This makes depth/offset changes fully reversible without touching the undo stack.
	// Track the bounding box of all restored positions for dirty region.
	const voxel::Region &volRegion = vol->region();
	math::Vec3i dirtyLo(volRegion.getUpperCorner());
	math::Vec3i dirtyHi(volRegion.getLowerCorner());
	for (const voxel::VoxelPosition &entry : _history) {
		vol->setVoxel(entry.pos, entry.voxel);
		dirtyLo = math::min(dirtyLo, entry.pos);
		dirtyHi = math::max(dirtyHi, entry.pos);
	}
	if (!_history.empty()) {
		wrapper.addToDirtyRegion(dirtyLo);
		wrapper.addToDirtyRegion(dirtyHi);
	}
	_history.clear();

	if (_parameters.depth == 0 || _parameters.face == voxel::FaceNames::Max || !_hasCachedSelection) {
		return;
	}

	const math::Axis axis = voxel::faceToAxis(_parameters.face);
	const int axisIdx = math::getIndexForAxis(axis);
	const int faceSign = voxel::isNegativeFace(_parameters.face) ? -1 : 1;

	const bool carving = _parameters.depth < 0;
	const int steps = math::abs(_parameters.depth);
	const int dirSign = carving ? -faceSign : faceSign;

	math::Vec3i dir(0);
	dir[axisIdx] = dirSign;

	static constexpr int NumAxes = 3;
	const int perp1 = (axisIdx + 1) % NumAxes;
	const int perp2 = (axisIdx + 2) % NumAxes;

	const float stepU = static_cast<float>(_parameters.offsetU) / steps;
	const float stepV = static_cast<float>(_parameters.offsetV) / steps;
	auto lateralShift = [&](int step) -> math::Vec3i {
		math::Vec3i shift(0);
		if (_parameters.offsetU != 0) {
			shift[perp1] = static_cast<int>(stepU * step);
		}
		if (_parameters.offsetV != 0) {
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
		if (_parameters.fillWalls) {
			for (int step = 0; step < steps; ++step) {
				const math::Vec3i shift = lateralShift(step);
				for (const WallCandidate &wc : _cachedWallCandidates) {
					const math::Vec3i sidePos = wc.basePos + wc.perpOffset + dir * step + shift;
					writeVoxel(wrapper, savedPositions, sidePos, _parameters.voxel);
				}
			}
		}
		// Loop 2: Carve out cached selected positions at each depth step
		for (int step = 0; step < steps; ++step) {
			const math::Vec3i shift = lateralShift(step);
			for (const math::Vec3i &selPos : _cachedSelectedPositions) {
				writeVoxel(wrapper, savedPositions, selPos + dir * step + shift, air);
			}
		}
	} else {
		// Positive depth: place selected voxels at each step from 1 to depth
		for (int step = 1; step <= steps; ++step) {
			const math::Vec3i shift = lateralShift(step);
			for (const math::Vec3i &selPos : _cachedSelectedPositions) {
				writeVoxel(wrapper, savedPositions, selPos + dir * step + shift, _parameters.voxel);
			}
		}
		// Include the original selected positions in the dirty region so that
		// auto-select will re-select them along with the newly extruded voxels
		wrapper.addToDirtyRegion(_cachedSelectedPositions);
	}
	prune(wrapper, savedPositions);
}

} // namespace voxedit
