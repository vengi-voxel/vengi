/**
 * @file
 */

#pragma once

#include "voxedit-util/modifier/brush/SnapshotHelper.h"
#include "core/ArrayLength.h"
#include "core/GLM.h"
#include "core/Optional.h"
#include "core/SharedPtr.h"
#include "core/String.h"
#include "palette/Palette.h"
#include "voxel/BitVolume.h"
#include "voxel/DynamicVoxelArray.h"
#include "voxel/Face.h"
#include "voxel/SparseVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeSculpt.h"

#include <climits>
#include <glm/vec3.hpp>

namespace voxedit {

class ModifierVolumeWrapper;

enum class SculptMode : uint8_t {
	Erode,
	Grow,
	Flatten,
	SmoothAdditive,
	SmoothErode,
	SmoothGaussian,
	SmoothWall,
	BridgeGap,
	SquashToPlane,
	ExtendPlane,
	Reskin,

	Max
};

struct SculptParameters {
	SculptMode mode = SculptMode::Erode;
	float strength = 0.5f;
	int iterations = 1;
	int heightThreshold = 1;
	bool preserveTopHeight = false;
	int trimPerStep = 1;
	int kernelSize = 4;
	float sigma = 4.0f;
	int smoothWallClearDepth = 256;
	voxelutil::SmoothWallInterp smoothWallInterp = voxelutil::SmoothWallInterp::InverseDistance;
	bool smoothWallFillHoles = true;
	voxel::FaceNames face = voxel::FaceNames::Max;
	int plane = 0;
	int removeAboveDepth = 0;
	bool extendOnly = true;
	bool removeOnly = false;
	int radius = 3;
	voxel::Voxel voxel;
	glm::ivec3 cursor{0};
	voxelutil::ReskinConfig reskin;
	core::SharedPtr<voxel::RawVolume> skin;
	core::Optional<palette::Palette> skinPalette;
};

class SculptOperation {
protected:
	SculptParameters _parameters;
	voxel::Region _cachedRegion = voxel::Region::InvalidRegion;
	bool _cachedRegionValid = false;
	bool _active = false;
	bool _hasSnapshot = false;
	bool _paramsDirty = true;

	// ExtendPlane parameters
	/** When set, ExtendPlane only removes voxels above the plane and never places new ones. */
	float _planeGradU = 0.0f;
	float _planeGradV = 0.0f;
	int _planeSeedU = 0;
	int _planeSeedV = 0;
	int _planeSeedH = 0;
	int _planeUAxis = 0;
	int _planeVAxis = 2;
	int _planeHeightAxis = 1;
	bool _planeFitted = false;
	glm::ivec3 _lastPaintPos{INT_MIN};

	// Original selected voxels captured at brush activation, stored as a flat array.
	// Using DynamicVoxelArray instead of SparseVolume avoids O(N) hash insertions.
	voxel::DynamicVoxelArray _snapshotEntries;
	// Selection bounding box at capture time
	voxel::Region _snapshotRegion;
	// Volume region lower corner at snapshot capture time (to detect region shifts)
	glm::ivec3 _capturedVolumeLower{0};

	// Per-evaluate bookkeeping: tracks positions written during a single evaluate()
	// call so the previous state can be restored before re-applying.
	// Using BitVolume + flat array instead of SparseVolume avoids O(N) hash chain traversal
	// per lookup/insert. For 5M+ entries this reduces history operations from ~30s to ~50ms.
	voxel::DynamicVoxelArray _historyEntries;
	voxel::BitVolume _historyBits;
	voxel::Region _historyRegion;

	// Cached data structures built from snapshot entries. Rebuilt when snapshot changes.
	// Avoids rebuilding O(N) hash maps and bit arrays every frame.
	bool _cachedBitVolumeValid = false;
	voxel::BitVolume _cachedBitVolume;
	voxel::SparseVolume _cachedVoxelMap;
	bool _cachedVoxelMapValid = false;

	void rebuildCachedBitVolume();
	void rebuildCachedVoxelMap();
	void captureSnapshot(const voxel::RawVolume *volume, const voxel::Region &region);
	void adjustSnapshotForRegionShift(const glm::ivec3 &delta);
	void applySculpt(ModifierVolumeWrapper &wrapper);
	void saveToHistory(voxel::RawVolume *volume, const glm::ivec3 &pos);
	void writeVoxel(ModifierVolumeWrapper &wrapper, const glm::ivec3 &pos, const voxel::Voxel &voxel);
	void fitPlaneFromSnapshot();
	void paintExtendPlane(ModifierVolumeWrapper &wrapper);

public:
	static constexpr bool requiresSelection = true;
	using Parameters = SculptParameters;
	static bool sameOperation(const Parameters &a, const Parameters &b) {
		return a.mode == b.mode && a.face == b.face && a.skin.get() == b.skin.get();
	}
	static bool modeNeedsFace(SculptMode mode);
	const Parameters &parameters() const {
		return _parameters;
	}
	bool setParameters(const Parameters &parameters);
	void prepare(const voxel::RawVolume *volume, const voxel::Region &region);
	void evaluate(ModifierVolumeWrapper &wrapper);
	bool hasSnapshot() const {
		return _hasSnapshot;
	}
	const palette::Palette *skinPalette() const {
		return _parameters.skinPalette.value();
	}
};

} // namespace voxedit
