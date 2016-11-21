#pragma once

#include "voxel/polyvox/RawVolume.h"
#include "io/File.h"

namespace voxel {

class VoxFileFormat {
protected:
	std::vector<glm::vec4> _palette;
	size_t _paletteSize = 0;

	const glm::vec4& getColor(const Voxel& voxel) const;
	const glm::vec4& getColor(VoxelType type) const;
	VoxelType findVoxelType(const glm::vec4& color) const;
	glm::vec4 findClosestMatch(const glm::vec4& color) const;
	glm::vec4 paletteColor(uint32_t index) const;
public:
	virtual ~VoxFileFormat() {
	}

	virtual RawVolume* load(const io::FilePtr& file) = 0;
	virtual bool save(const RawVolume* volume, const io::FilePtr& file) = 0;
};

}
