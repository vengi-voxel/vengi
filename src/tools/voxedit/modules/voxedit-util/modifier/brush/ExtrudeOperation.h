/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include "core/collection/Buffer.h"
#include "voxel/Voxel.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicSet.h"
#include "voxel/DynamicVoxelArray.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"

namespace voxedit {

class ModifierVolumeWrapper;


/** Geometry and captured voxel data used by ExtrudeBrush. */
class ExtrudeOperation {
protected:
	voxel::FaceNames _face = voxel::FaceNames::Max;
	int _depth = 0;
	int _offsetU = 0; // lateral offset along first perpendicular axis
	int _offsetV = 0; // lateral offset along second perpendicular axis
	bool _fillWalls = true;
	// For each position overwritten by the current extrude session, stores the original voxel.
	// Cleared in reset() when the brush is deselected.
	voxel::DynamicVoxelArray _history;

	using PositionSet = core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>>;

	// Cached selected voxel positions, captured once on first generate() call.
	// Re-used for all subsequent depth/offset changes to prevent selection bleeding.
	core::DynamicArray<glm::ivec3> _cachedSelectedPositions;
	voxel::Region _cachedSelectedBBox;
	bool _hasCachedSelection = false;

	// Wall candidate: a selected voxel with a solid non-selected perpendicular neighbor.
	// The wall extends from basePos + perpOffset along the extrusion direction.
	// This variable exists to debug complex 3D extrusion where walls kept overwriting each other and causing holes. The candidates are cached to be able to analyze the final wall configuration in endBrush() and adjust the wall filling logic if necessary.
	struct WallCandidate {
		glm::ivec3 basePos;
		glm::ivec3 perpOffset;
	};
	core::DynamicArray<WallCandidate> _cachedWallCandidates;

	// Cached bounding box of selected (FlagOutline) voxels, computed in preExecute().
	// Used by calcRegion() to return a tight preview region instead of the full volume.
	voxel::Region _cachedSelBBox;
	bool _cachedSelBBoxValid = false;

	// Volume region lower corner when cache was captured (to detect region shifts)
	glm::ivec3 _capturedVolumeLower{0};

	void cacheSelection(const voxel::RawVolume *vol, const voxel::Region &volRegion);
	void adjustCacheForRegionShift(const glm::ivec3 &delta);
	void writeVoxel(ModifierVolumeWrapper &wrapper, PositionSet &savedPositions, const glm::ivec3 &pos, const voxel::Voxel &newVoxel);
	void evaluate(ModifierVolumeWrapper &wrapper, const voxel::Voxel &voxel);

};

} // namespace voxedit
