/**
 * @file
 */

#pragma once

#include "voxel/Region.h"
#include "voxel/SparseVolume.h"
#include "voxel/Voxel.h"

#include <glm/vec3.hpp>

namespace voxel {
class RawVolume;
class RawVolumeWrapper;
} // namespace voxel

namespace voxedit {

class ModifierVolumeWrapper;

/**
 * @brief Manages snapshot capture and history tracking for brushes that modify voxels
 * with a preview/commit lifecycle (Extrude, Transform, Sculpt).
 *
 * The pattern:
 * 1. On activation: capture original selected voxels into a snapshot
 * 2. On each generate: restore history, clear it, re-apply operation from snapshot
 * 3. On deactivation/commit: create single undo entry
 * 4. On cancel: revert all changes from history
 *
 * The snapshot stores the original voxels. The history stores the original state
 * of each position modified during generate(), enabling per-frame undo without
 * touching the undo stack.
 */
class SnapshotHelper {
private:
	// Original selected voxels captured at brush activation
	voxel::SparseVolume _snapshot;
	// Per-generate bookkeeping: tracks positions modified during a generate() call
	// so the previous state can be restored before re-applying.
	// Stores the original voxel at each modified position (with empty voxel storage enabled).
	voxel::SparseVolume _history;
	// Selection bounding box at capture time
	voxel::Region _snapshotRegion = voxel::Region::InvalidRegion;
	// Volume region lower corner at snapshot capture time (to detect region shifts)
	glm::ivec3 _capturedVolumeLower{0};
	bool _hasSnapshot = false;

public:
	SnapshotHelper() {
		_history.setStoreEmptyVoxels(true);
	}

	/**
	 * @brief Capture selected voxels (FlagOutline) from the volume into the snapshot
	 */
	void captureSnapshot(const voxel::RawVolume *volume, const voxel::Region &volRegion);

	/**
	 * @brief Adjust snapshot and history positions when the volume region shifts
	 * (e.g. node was moved)
	 */
	void adjustForRegionShift(const glm::ivec3 &delta);

	/**
	 * @brief Save the current voxel at @p pos into history (if not already tracked)
	 */
	void saveToHistory(voxel::RawVolume *vol, const glm::ivec3 &pos);

	/**
	 * @brief Write a voxel at @p pos, saving the original to history first
	 */
	void writeVoxel(ModifierVolumeWrapper &wrapper, const glm::ivec3 &pos, const voxel::Voxel &newVoxel);

	/**
	 * @brief Restore all history entries into the volume (used when cancelling)
	 * @return The dirty region of all restored voxels
	 */
	voxel::Region revertChanges(voxel::RawVolume *volume);

	/**
	 * @brief Restore history entries into the volume and clear history.
	 * Used at the start of generate() to undo the previous frame's changes
	 * before re-applying the operation from the snapshot.
	 */
	void restoreHistory(voxel::RawVolume *vol, ModifierVolumeWrapper &wrapper);

	/**
	 * @brief Clear all snapshot and history state
	 */
	void clear();

	bool hasSnapshot() const {
		return _hasSnapshot;
	}

	const voxel::Region &snapshotRegion() const {
		return _snapshotRegion;
	}

	const glm::ivec3 &capturedVolumeLower() const {
		return _capturedVolumeLower;
	}

	const voxel::SparseVolume &snapshot() const {
		return _snapshot;
	}

	voxel::SparseVolume &snapshot() {
		return _snapshot;
	}

	const voxel::SparseVolume &history() const {
		return _history;
	}

	size_t snapshotVoxelCount() const {
		return _snapshot.size();
	}

	bool historyEmpty() const {
		return _history.empty();
	}
};

} // namespace voxedit
