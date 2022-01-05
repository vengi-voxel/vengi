/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxel {
/**
 * @brief Voxel sprite format used by the SLAB6 editor, voxlap and Ace of Spades
 */
class KV6Format : public Format {
public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& volumes) override;
	bool saveGroups(const SceneGraph& volumes, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
