/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "core/io/File.h"
#include "core/String.h"

namespace voxel {
/**
 * @brief Voxel sprite format used by the SLAB6 editor, voxlap and Ace of Spades
 */
class KV6Format : public VoxFileFormat {
public:
	bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
