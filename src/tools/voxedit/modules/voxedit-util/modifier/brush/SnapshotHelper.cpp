/**
 * @file
 */

#include "SnapshotHelper.h"
#include "app/App.h"
#include "app/ForParallel.h"
#include "core/Common.h"
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

	// Parallel scan: split Z-axis into chunks, each thread collects into its own array.
	// No locks needed - each thread writes to its own DynamicVoxelArray.
	const int zStart = volRegion.getLowerZ();
	const int zEnd = volRegion.getUpperZ() + 1;
	const int zRange = zEnd - zStart;

	if (zRange <= 0) {
		_hasSnapshot = false;
		return;
	}

	// Determine chunk count based on available threads
	const int threadCount = app::App::getInstance()->threads();
	const int chunkCount = (threadCount > 1) ? core_min(threadCount, zRange) : 1;

	core::DynamicArray<voxel::DynamicVoxelArray> threadResults;
	threadResults.resize(chunkCount);

	app::for_parallel(0, chunkCount, [&](int chunkStart, int chunkEnd) {
		for (int chunk = chunkStart; chunk < chunkEnd; ++chunk) {
			const int sliceStart = zStart + (zRange * chunk) / chunkCount;
			const int sliceEnd = zStart + (zRange * (chunk + 1)) / chunkCount;
			const voxel::Region subRegion(
				volRegion.getLowerX(), volRegion.getLowerY(), sliceStart,
				volRegion.getUpperX(), volRegion.getUpperY(), sliceEnd - 1);
			voxelutil::visitVolume(*volume, subRegion,
				[&threadResults, chunk](int x, int y, int z, const voxel::Voxel &voxel) {
					threadResults[chunk].push_back({glm::ivec3(x, y, z), voxel});
				},
				voxelutil::VisitSolidOutline());
		}
	});

	// Merge thread-local results
	size_t totalCount = 0;
	for (const auto &arr : threadResults) {
		totalCount += arr.size();
	}
	if (totalCount == 0) {
		_hasSnapshot = false;
		return;
	}

	voxel::DynamicVoxelArray collected;
	collected.reserve(totalCount);
	for (auto &arr : threadResults) {
		for (const voxel::VoxelPosition &vp : arr) {
			collected.push_back(vp);
		}
	}

	// Compute bounding box
	glm::ivec3 selLo(collected[0].pos);
	glm::ivec3 selHi(collected[0].pos);
	for (const voxel::VoxelPosition &vp : collected) {
		selLo = glm::min(selLo, vp.pos);
		selHi = glm::max(selHi, vp.pos);
	}

	// Batch insert into sparse volume (sorts by chunk for cache efficiency)
	_snapshot.insertBatch(collected);

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
