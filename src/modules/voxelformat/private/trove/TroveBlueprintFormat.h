/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Trove @c .blueprint (kiwib) format
 *
 * Native Trove voxel format used by Glyph/Kiwi. Magic is @c kiwib followed by a
 * little-endian @c uint32 version.
 *
 * - v3 / v4: uncompressed voxel list (uleb128 count, zigzag coords, type, BGRA)
 * - v5: zlib-compressed body with sparse linear indices (current game format)
 *
 * Based on reverse-engineering used by BetterTroveTools / KiwiAPI
 * (https://github.com/AallynReed/BetterTroveTools,
 * https://github.com/AallynReed/KiwiAPI) and Troxel community tooling.
 *
 * @ingroup Formats
 */
class TroveBlueprintFormat : public RGBASinglePaletteFormat {
protected:
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
						scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
						const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	bool singleVolume() const override {
		return true;
	}

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Trove Blueprint",
									   "",
									   {"blueprint"},
									   {"kiwib"},
									   VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE | VOX_FORMAT_FLAG_RGB};
		return f;
	}
};

} // namespace voxelformat
