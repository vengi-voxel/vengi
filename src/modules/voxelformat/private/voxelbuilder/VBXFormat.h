/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief VoxelBuilder format
 *
 * The format is ini-based and supports embedded glb files
 *
 * @ingroup Formats
 */
class VBXFormat : public RGBASinglePaletteFormat {
private:
	bool loadGLB(const core::String &voxels, scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) const;

protected:
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
						scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
						const LoadContext &ctx) override;

public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override {
		return false;
	}

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"VoxelBuilder", "", {"vbx"}, {"; Voxel Builder file format (VBX)"}, VOX_FORMAT_FLAG_RGB};
		return f;
	}
};

} // namespace voxelformat
