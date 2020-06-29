/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"
#include "core/io/File.h"
#include "VoxelVolumes.h"
#include <glm/fwd.hpp>
#include <vector>

namespace voxel {

class VoxFileFormat {
protected:
	std::vector<uint8_t> _palette;
	size_t _paletteSize = 0;

	const glm::vec4& getColor(const Voxel& voxel) const;
	glm::vec4 findClosestMatch(const glm::vec4& color) const;
	uint8_t findClosestIndex(const glm::vec4& color) const;
	/**
	 * @brief Maps a custum palette index to our own 256 color palette by a closest match
	 */
	uint8_t convertPaletteIndex(uint32_t paletteIndex) const;
	RawVolume* merge(const VoxelVolumes& volumes) const;
public:
	virtual ~VoxFileFormat() = default;

	virtual const char *magic() const;
	virtual bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) = 0;
	virtual RawVolume* load(const io::FilePtr& file);
	virtual bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) = 0;
	virtual bool save(const RawVolume* volume, const io::FilePtr& file);
};


}
