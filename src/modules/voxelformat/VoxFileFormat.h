/**
 * @file
 */

#pragma once

#include "voxel/polyvox/RawVolume.h"
#include "io/File.h"
#include <vector>

namespace voxel {

class VoxFileFormat {
protected:
	std::vector<uint8_t> _palette;
	size_t _paletteSize = 0;

	const glm::vec4& getColor(const Voxel& voxel) const;
	glm::vec4 findClosestMatch(const glm::vec4& color) const;
	uint8_t findClosestIndex(const glm::vec4& color) const;
	uint8_t convertPaletteIndex(uint32_t paletteIndex) const;
public:
	virtual ~VoxFileFormat() = default;

	virtual std::vector<RawVolume*> loadGroups(const io::FilePtr& file) = 0;
	RawVolume* load(const io::FilePtr& file);
	virtual bool save(const RawVolume* volume, const io::FilePtr& file) = 0;
};

}
