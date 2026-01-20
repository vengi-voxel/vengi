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
	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

	/**
	 * @brief KWALK kv6 sprite animations
	 *
	 * The kfa file and the kv6 file must have the same basename
	 *
	 * https://github.com/Ericson2314/Voxlap/blob/no-asm/share/documentation/kwalkhlp.txt
	 * https://github.com/Ericson2314/Voxlap/blob/no-asm/source/kwalk.cpp
	 * Example files at https://github.com/Ericson2314/Voxlap/tree/no-asm/share (anasplit.kfa)
	 */
	bool loadKFA(const core::String &filename, const io::ArchivePtr &archive, const voxel::RawVolume *volume,
				 scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette);

public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	bool singleVolume() const override {
		return true;
	}

	static const io::FormatDescription &format() {
		static io::FormatDescription f{
			"AceOfSpades", "", {"kv6"}, {"Kvxl"}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
		return f;
	}

};

} // namespace voxelformat
