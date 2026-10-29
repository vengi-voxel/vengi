/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include "core/collection/Buffer.h"
#include "voxel/Voxel.h"
#include "voxel/Face.h"
#include "core/Optional.h"
#include "core/ScopedPtr.h"
#include "palette/Palette.h"
#include "voxel/BitVolume.h"
#include "voxel/DynamicVoxelArray.h"
#include "voxel/RawVolume.h"
#include "voxel/SparseVolume.h"
#include "voxelutil/VolumeSculpt.h"
#include <climits>

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

/** Geometry and captured voxel data used by SculptBrush. */
class SculptOperation {
public:
	static constexpr int MaxSmoothWallClearDepth = 256;

protected:
	SculptMode _sculptMode = SculptMode::Erode;
	float _strength = 0.5f;
	int _iterations = 1;
	int _heightThreshold = 1;
	bool _preserveTopHeight = false;
	int _trimPerStep = 1;
	int _kernelSize = 4;
	float _sigma = 4.0f;
	int _smoothWallClearDepth = MaxSmoothWallClearDepth; ///< 0 = don't clear, max = clear entire depth
	voxelutil::SmoothWallInterp _smoothWallInterp = voxelutil::SmoothWallInterp::InverseDistance;
	bool _smoothWallFillHoles = true; ///< Fill enclosed empty areas with interpolated heights
	voxel::FaceNames _flattenFace = voxel::FaceNames::Max;
	int _squashPlaneCoord = 0;
	bool _active = false;
	bool _hasSnapshot = false;
	bool _paramsDirty = true;

	// ExtendPlane parameters
	int _removeAboveDepth = 0;
	bool _extendOnly = true;
	/** When set, ExtendPlane only removes voxels above the plane and never places new ones. */
	bool _removeOnly = false;
	int _brushRadius = 3;
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

	// Reskin parameters
	voxelutil::ReskinConfig _reskinConfig;
	const voxel::RawVolume *_skinVolume = nullptr;
	core::ScopedPtr<voxel::RawVolume> _ownedSkinVolume;
	core::Optional<palette::Palette> _skinPalette;

	// Original selected voxels captured at brush activation, stored as a flat array.
	// Using DynamicVoxelArray instead of SparseVolume avoids O(N) hash insertions.
	voxel::DynamicVoxelArray _snapshotEntries;
	// Selection bounding box at capture time
	voxel::Region _snapshotRegion;
	// Volume region lower corner at snapshot capture time (to detect region shifts)
	glm::ivec3 _capturedVolumeLower{0};

	// Per-generate bookkeeping: tracks positions written during a single generate()
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

	// Cached region for preview
	voxel::Region _cachedRegion;
	bool _cachedRegionValid = false;

	void captureSnapshot(const voxel::RawVolume *volume, const voxel::Region &volRegion);
	void adjustSnapshotForRegionShift(const glm::ivec3 &delta);
	void applySculpt(ModifierVolumeWrapper &wrapper, const voxel::Voxel &voxel);
	void saveToHistory(voxel::RawVolume *vol, const glm::ivec3 &pos);
	void writeVoxel(ModifierVolumeWrapper &wrapper, const glm::ivec3 &pos, const voxel::Voxel &voxel);
	void fitPlaneFromSnapshot();
	void paintExtendPlane(ModifierVolumeWrapper &wrapper, const voxel::Voxel &voxel, const glm::ivec3 &cursor);
};

} // namespace voxedit
