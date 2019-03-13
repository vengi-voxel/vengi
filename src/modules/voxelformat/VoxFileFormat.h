/**
 * @file
 */

#pragma once

#include "voxel/polyvox/RawVolume.h"
#include "io/File.h"
#include <vector>

namespace voxel {

struct VoxelVolume {
	VoxelVolume(RawVolume* _volume = nullptr, const std::string& _name = "", bool _visible = true) :
			volume(_volume), name(_name), visible(_visible) {
	}
	RawVolume* volume;
	std::string name;
	bool visible;
};

using VoxelVolumes = std::vector<VoxelVolume>;

class VoxFileFormat {
protected:
	std::vector<uint8_t> _palette;
	size_t _paletteSize = 0;

	const glm::vec4& getColor(const Voxel& voxel) const;
	glm::vec4 findClosestMatch(const glm::vec4& color) const;
	uint8_t findClosestIndex(const glm::vec4& color) const;
	uint8_t convertPaletteIndex(uint32_t paletteIndex) const;
	RawVolume* merge(const VoxelVolumes& volumes) const;
public:
	virtual ~VoxFileFormat() = default;

	virtual VoxelVolumes loadGroups(const io::FilePtr& file) = 0;
	virtual RawVolume* load(const io::FilePtr& file);
	virtual bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) = 0;
	virtual bool save(const RawVolume* volume, const io::FilePtr& file);
};


}
