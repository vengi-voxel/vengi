/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @ingroup Formats
 */
class XRawFormat : public RGBASinglePaletteFormat {
protected:
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
						scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
						const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;
	virtual int emptyPaletteIndex() {
		return 0;
	}
public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	bool singleVolume() const override {
		return true;
	}
};

} // namespace voxelformat
