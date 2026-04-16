/**
 * @file
 */

#include "SnapshotHelper.h"
#include "core/Trace.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/DynamicVoxelArray.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

void SnapshotHelper::captureSnapshot(const voxel::RawVolume *volume, const voxel::Region &volRegion) {
	core_trace_scoped(SnapshotHelperCaptureSnapshot);
	_snapshot.clear();
	glm::ivec3 selLo(volRegion.getUpperCorner());
	glm::ivec3 selHi(volRegion.getLowerCorner());

	voxelutil::visitVolume(
		*volume, volRegion,
		[&](int x, int y, int z, const voxel::Voxel &voxel) {
			const glm::ivec3 pos(x, y, z);
			_snapshot.setVoxel(pos, voxel);
			selLo = glm::min(selLo, pos);
			selHi = glm::max(selHi, pos);
		},
		voxelutil::VisitSolidOutline());

	if (_snapshot.empty()) {
		_hasSnapshot = false;
		return;
	}

	_snapshotRegion = voxel::Region(selLo, selHi);
	_capturedVolumeLower = volRegion.getLowerCorner();
	_hasSnapshot = true;
}

void SnapshotHelper::adjustForRegionShift(const glm::ivec3 &delta) {
	core_trace_scoped(SnapshotHelperAdjustForRegionShift);
	// Shift snapshot entries
	voxel::DynamicVoxelArray entries;
	_snapshot.copyTo(entries, _snapshotRegion);
	_snapshot.clear();
	for (const voxel::VoxelPosition &e : entries) {
		_snapshot.setVoxel(delta + e.pos, e.voxel);
	}

	_snapshotRegion.shift(delta.x, delta.y, delta.z);
	_capturedVolumeLower += delta;

	// Shift history entries
	struct EntryCollector {
		voxel::DynamicVoxelArray *entries;
		glm::ivec3 delta;
		bool setVoxel(int x, int y, int z, const voxel::Voxel &voxel) {
			entries->push_back({glm::ivec3(x + delta.x, y + delta.y, z + delta.z), voxel});
			return true;
		}
	};
	voxel::DynamicVoxelArray historyEntries;
	historyEntries.reserve(_history.size());
	EntryCollector collector{&historyEntries, delta};
	_history.copyTo(collector);
	_history.clear();
	for (const voxel::VoxelPosition &e : historyEntries) {
		_history.setVoxel(e.pos, e.voxel);
	}
}

void SnapshotHelper::saveToHistory(voxel::RawVolume *vol, const glm::ivec3 &pos) {
	if (_history.hasVoxel(pos)) {
		return;
	}
	_history.setVoxel(pos, vol->voxel(pos));
}

void SnapshotHelper::writeVoxel(ModifierVolumeWrapper &wrapper, const glm::ivec3 &pos,
								const voxel::Voxel &newVoxel) {
	voxel::RawVolume *volume = wrapper.volume();
	if (!volume->region().containsPoint(pos)) {
		return;
	}
	saveToHistory(volume, pos);
	if (volume->setVoxel(pos, newVoxel)) {
		wrapper.addToDirtyRegion(pos);
	}
}

voxel::Region SnapshotHelper::revertChanges(voxel::RawVolume *volume) {
	voxel::RawVolumeWrapper wrapper(volume);
	_history.copyTo(wrapper);
	_history.clear();
	return wrapper.dirtyRegion();
}

void SnapshotHelper::restoreHistory(voxel::RawVolume *vol, ModifierVolumeWrapper &wrapper) {
	struct HistoryRestorer {
		voxel::RawVolume *vol;
		ModifierVolumeWrapper *wrapper;
		bool setVoxel(int x, int y, int z, const voxel::Voxel &voxel) {
			if (vol->setVoxel(x, y, z, voxel)) {
				wrapper->addToDirtyRegion(glm::ivec3(x, y, z));
			}
			return true;
		}
	};
	HistoryRestorer restorer{vol, &wrapper};
	_history.copyTo(restorer);
	_history.clear();
}

void SnapshotHelper::clear() {
	_snapshot.clear();
	_history.clear();
	_snapshotRegion = voxel::Region::InvalidRegion;
	_capturedVolumeLower = glm::ivec3(0);
	_hasSnapshot = false;
}

} // namespace voxedit
