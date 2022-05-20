/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {
/**
 * @brief Voxel sprite format used by the SLAB6 editor, voxlap and Ace of Spades
 */
class KV6Format : public PaletteFormat {
protected:
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette) override;
public:
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
