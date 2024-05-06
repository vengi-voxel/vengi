/**
 * @file
 */

#include "palette/Palette.h"
#include "voxel/RawVolume.h"

namespace voxel {

class VoxelData {
private:
	bool _disposeAfterUse = false;

public:
	voxel::RawVolume *volume = nullptr;
	palette::Palette *palette = nullptr;

	VoxelData() = default;
	VoxelData(const voxel::RawVolume *v, const palette::Palette *p, bool disposeAfterUse);
	VoxelData(const voxel::RawVolume *v, const palette::Palette &p, bool disposeAfterUse);
	VoxelData(voxel::RawVolume *v, const palette::Palette *p, bool disposeAfterUse);
	VoxelData(voxel::RawVolume *v, const palette::Palette &p, bool disposeAfterUse);
	VoxelData(const VoxelData &v);
	VoxelData(VoxelData &&v);
	~VoxelData();

	operator bool() const {
		return volume != nullptr && palette != nullptr;
	}

	bool dispose() const;

	VoxelData &operator=(const VoxelData &v);

	VoxelData &operator=(VoxelData &&v);
};

} // namespace voxel
