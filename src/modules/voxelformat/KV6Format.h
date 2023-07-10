/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {
/**
 * @brief Voxel sprite format used by the SLAB6 editor, voxlap and Ace of Spades
 *
 * https://github.com/vuolen/slab6-mirror/blob/master/slab6.txt
 * https://gist.github.com/falkreon/8b873ec6797ffad247375fc73614fd08
 *
 * @ingroup Formats
 */
class KV6Format : public PaletteFormat {
protected:
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
						   scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override;

public:
	size_t loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
					   const LoadContext &ctx) override;

	bool singleVolume() const override {
		return true;
	}
};

} // namespace voxelformat
