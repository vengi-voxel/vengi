/**
 * @file
 */

#pragma once

#include "math/Vec3f.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicSet.h"
#include "voxel/DynamicVoxelArray.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "math/Vec3i.h"

namespace voxedit {

class ModifierVolumeWrapper;

struct ExtrudeParameters {
	voxel::FaceNames face = voxel::FaceNames::Max;
	int depth = 0;
	int offsetU = 0;
	int offsetV = 0;
	bool fillWalls = true;
	voxel::Voxel voxel;
	bool operator==(const ExtrudeParameters &other) const {
		return face == other.face && depth == other.depth && offsetU == other.offsetU && offsetV == other.offsetV &&
			   fillWalls == other.fillWalls && voxel == other.voxel;
	}
};

class ExtrudeOperation {
protected:
	ExtrudeParameters _parameters;
	// For each position overwritten by the current extrude session, stores the original voxel.
	// Cleared by ExtrudeBrush::clearOperation() when the draft is discarded.
	voxel::DynamicVoxelArray _history;

	using PositionSet = core::DynamicSet<math::Vec3i, 1031, math::Vec3iHash>;

	// Cached selected voxel positions, captured once on first prepare() call.
	// Re-used for all subsequent depth/offset changes to prevent selection bleeding.
	core::DynamicArray<math::Vec3i> _cachedSelectedPositions;
	voxel::Region _cachedSelectedBBox;
	bool _hasCachedSelection = false;

	// Wall candidate: a selected voxel with a solid non-selected perpendicular neighbor.
	// The wall extends from basePos + perpOffset along the extrusion direction.
	// This variable exists to debug complex 3D extrusion where walls kept overwriting each other and causing holes. The
	// candidates are cached to be able to analyze the final wall configuration after evaluate() and adjust the wall
	// filling logic if necessary.
	struct WallCandidate {
		math::Vec3i basePos;
		math::Vec3i perpOffset;
	};
	core::DynamicArray<WallCandidate> _cachedWallCandidates;

	// Cached bounding box of selected (FlagOutline) voxels, computed in prepare().
	// Used by calcRegion() to return a tight preview region instead of the full volume.
	voxel::Region _cachedSelBBox;
	bool _cachedSelBBoxValid = false;

	// Volume region lower corner when cache was captured (to detect region shifts)
	math::Vec3i _capturedVolumeLower{0};

	void cacheSelection(const voxel::RawVolume *volume, const voxel::Region &region);
	void adjustCacheForRegionShift(const math::Vec3i &delta);
	void writeVoxel(ModifierVolumeWrapper &wrapper, PositionSet &saved, const math::Vec3i &pos,
					const voxel::Voxel &voxel);
	void prune(ModifierVolumeWrapper &wrapper, PositionSet &saved);

public:
	static constexpr bool requiresSelection = true;
	using Parameters = ExtrudeParameters;
	static bool sameOperation(const Parameters &a, const Parameters &b) {
		return a.face == b.face;
	}
	const Parameters &parameters() const {
		return _parameters;
	}
	bool setParameters(const Parameters &parameters);
	void prepare(const voxel::RawVolume *volume, const voxel::Region &region);
	void evaluate(ModifierVolumeWrapper &wrapper);
	bool hasSnapshot() const {
		return _hasCachedSelection;
	}
	const voxel::Region &snapshotRegion() const {
		return _cachedSelectedBBox;
	}
};

} // namespace voxedit
