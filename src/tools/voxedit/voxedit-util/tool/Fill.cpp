/**
 * @file
 */

#include "Fill.h"
#include "voxel/VolumeMerger.h"
#include "voxel/VolumeCropper.h"

namespace voxedit {
namespace tool {

bool aabb(voxel::RawVolumeWrapper& target, const glm::ivec3& mins, const glm::ivec3& maxs, const voxel::Voxel& voxel, ModifierType modifierType, const Selection& selection, voxel::Region* modifiedRegion) {
	const bool deleteVoxels = (modifierType & ModifierType::Delete) == ModifierType::Delete;
	const bool overwrite = (modifierType & ModifierType::Place) == ModifierType::Place && deleteVoxels;
	const bool update = (modifierType & ModifierType::Update) == ModifierType::Update;
	voxel::Voxel placeVoxel = voxel;
	if (!overwrite && deleteVoxels) {
		placeVoxel = voxel::createVoxel(voxel::VoxelType::Air, 0);
	}
	glm::ivec3 modifiedMins((std::numeric_limits<int>::max)());
	glm::ivec3 modifiedMaxs((std::numeric_limits<int>::min)());
	glm::ivec3 operateMins = mins;
	glm::ivec3 operateMaxs = maxs;
	if (!selection.isValid()) {
		operateMins = (glm::max)(mins, selection.getLowerCorner());
		operateMaxs = (glm::min)(maxs, selection.getUpperCorner());
	}
	int cnt = 0;
	for (int32_t z = operateMins.z; z <= operateMaxs.z; ++z) {
		for (int32_t y = operateMins.y; y <= operateMaxs.y; ++y) {
			for (int32_t x = operateMins.x; x <= operateMaxs.x; ++x) {
				bool place = overwrite || deleteVoxels;
				if (!place) {
					const bool empty = isAir(target.voxel(x, y, z).getMaterial());
					if (update) {
						place = !empty;
					} else {
						place = empty;
					}
					if (!place) {
						continue;
					}
				}
				if (!target.setVoxel(x, y, z, placeVoxel)) {
					continue;
				}
				++cnt;
				modifiedMins.x = core_min(modifiedMins.x, x);
				modifiedMins.y = core_min(modifiedMins.y, y);
				modifiedMins.z = core_min(modifiedMins.z, z);

				modifiedMaxs.x = core_max(modifiedMaxs.x, x);
				modifiedMaxs.y = core_max(modifiedMaxs.y, y);
				modifiedMaxs.z = core_max(modifiedMaxs.z, z);
			}
		}
	}
	if (cnt <= 0) {
		return false;
	}
	if (modifiedRegion != nullptr) {
		*modifiedRegion = voxel::Region(modifiedMins, modifiedMaxs);
	}
	return true;
}

voxel::RawVolume* copy(const voxel::RawVolume *volume, const Selection &selection) {
	if (!selection.isValid()) {
		Log::debug("Copy failed: Source region is invalid: %s", selection.toString().c_str());
		return nullptr;
	}

	const voxel::Region& volumeRegion = volume->region();

	voxel::Region srcRegion(selection);
	srcRegion.cropTo(volumeRegion);

	voxel::RawVolume* v = new voxel::RawVolume(srcRegion);
	const glm::ivec3& mins = srcRegion.getLowerCorner();
	const glm::ivec3& maxs = srcRegion.getUpperCorner();
	voxel::RawVolume::Sampler sampler(volume);
	for (int32_t x = mins.x; x <= maxs.x; ++x) {
		for (int32_t y = mins.y; y <= maxs.y; ++y) {
			if (!sampler.setPosition(x, y, mins.z)) {
				continue;
			}
			for (int32_t z = mins.z; z <= maxs.z; ++z) {
				v->setVoxel(x, y, z, sampler.voxel());
				sampler.movePositiveZ();
				if (!sampler.currentPositionValid()) {
					break;
				}
			}
		}
	}
	return v;
}

voxel::RawVolume* cut(voxel::RawVolume *volume, const Selection &selection, voxel::Region& modifiedRegion) {
	if (!selection.isValid()) {
		Log::debug("Cut failed: Source region is invalid: %s", selection.toString().c_str());
		return nullptr;
	}

	voxel::RawVolume* v = copy(volume, selection);
	if (v == nullptr) {
		return nullptr;
	}

	const glm::ivec3& mins = selection.getLowerCorner();
	const glm::ivec3& maxs = selection.getUpperCorner();
	static constexpr voxel::Voxel AIR;
	voxel::RawVolumeWrapper wrapper(volume);
	for (int32_t x = mins.x; x <= maxs.x; ++x) {
		for (int32_t y = mins.y; y <= maxs.y; ++y) {
			for (int32_t z = mins.z; z <= maxs.z; ++z) {
				wrapper.setVoxel(x, y, z, AIR);
			}
		}
	}
	modifiedRegion = wrapper.dirtyRegion();

	return v;
}

void paste(voxel::RawVolume* out, const voxel::RawVolume* in, const glm::ivec3& referencePosition, voxel::Region& modifiedRegion) {
	voxel::Region destReg = out->region();
	destReg.shift(referencePosition);
	voxel::Region sourceReg = in->region();

	if (!destReg.isValid()) {
		Log::debug("Paste failed: Destination region is invalid: %s", destReg.toString().c_str());
		return;
	}

	voxel::RawVolume::Sampler srcSampler(in);
	voxel::RawVolume::Sampler destSampler(out);
	const glm::ivec3& srcMins = sourceReg.getLowerCorner();
	const glm::ivec3& srcMaxs = sourceReg.getUpperCorner();
	const glm::ivec3& destMins = destReg.getLowerCorner();

	glm::ivec3 modifiedMins((std::numeric_limits<int>::max)());
	glm::ivec3 modifiedMaxs((std::numeric_limits<int>::min)());

	for (int32_t x = srcMins.x, destX = destMins.x; x <= srcMaxs.x; ++x, ++destX) {
		for (int32_t y = sourceReg.getLowerY(), destY = destMins.y; y <= sourceReg.getUpperY(); ++y, ++destY) {
			srcSampler.setPosition(x, y, srcMins.z);
			if (!destSampler.setPosition(destX, destY, destMins.z)) {
				continue;
			}
			for (int32_t z = srcMins.z; z <= srcMaxs.z; ++z) {
				const voxel::Voxel& voxel = srcSampler.voxel();
				if (destSampler.setVoxel(voxel)) {
					modifiedMins = (glm::min)(modifiedMins, destSampler.position());
					modifiedMaxs = (glm::max)(modifiedMaxs, destSampler.position());
				}
				srcSampler.movePositiveZ();
				destSampler.movePositiveZ();
				if (!destSampler.currentPositionValid()) {
					break;
				}
			}
		}
	}

	modifiedRegion = voxel::Region(modifiedMins, modifiedMaxs);
	Log::debug("Pasted %s", modifiedRegion.toString().c_str());
}

}
}
