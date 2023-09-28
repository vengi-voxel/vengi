/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"
#include "voxelformat/Format.h"

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
	bool readColor(io::SeekableReadStream &stream, core::RGBA &color) const;
	bool writeColor(io::SeekableWriteStream &stream, core::RGBA color) const;

	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
						   scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override;

	/**
	 * @brief KWALK kv6 sprite animations
	 *
	 * The kfa file and the kv6 file must have the same basename
	 *
	 * https://github.com/Ericson2314/Voxlap/blob/no-asm/share/documentation/kwalkhlp.txt
	 * Example files at https://github.com/Ericson2314/Voxlap/tree/no-asm/share (anasplit.kfa)
	 */
	bool loadKFA(const core::String &filename, const voxel::RawVolume *volume, scenegraph::SceneGraph &sceneGraph,
				 const voxel::Palette &palette);

public:
	size_t loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
					   const LoadContext &ctx) override;

	bool singleVolume() const override {
		return true;
	}
};

} // namespace voxelformat
