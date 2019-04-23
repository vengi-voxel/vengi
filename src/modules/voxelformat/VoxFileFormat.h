/**
 * @file
 */

#pragma once

#include "voxel/polyvox/RawVolume.h"
#include "io/File.h"
#include <vector>

namespace voxel {

struct VoxelVolume {
	VoxelVolume(RawVolume* _volume = nullptr, const std::string& _name = "",
			bool _visible = true) :
			volume(_volume), name(_name), visible(_visible) {
		if (volume != nullptr) {
			pivot = volume->region().getCentre();
		} else {
			pivot = glm::zero<glm::ivec3>();
		}
	}
	VoxelVolume(RawVolume* _volume, const std::string& _name,
			bool _visible,
			const glm::ivec3& _pivot) :
			volume(_volume), name(_name), visible(_visible), pivot(_pivot) {
	}
	RawVolume* volume;
	std::string name;
	bool visible;
	glm::ivec3 pivot;
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
