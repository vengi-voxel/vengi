/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief EveryGraph Voxel3D format (v3b is compressed - see http://advsys.net/ken/util/v3b2vox.zip)
 *
 * @ingroup Formats
 */
class V3AFormat : public RGBAFormat {
protected:
	bool loadFromStream(const core::String &filename, io::ReadStream *stream, scenegraph::SceneGraph &sceneGraph,
						const palette::Palette &palette, const LoadContext &ctx);
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const palette::Palette &palette, const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;
	bool saveToStream(const scenegraph::SceneGraph &sceneGraph, io::WriteStream *stream, const SaveContext &ctx);

public:
	bool singleVolume() const override {
		return true;
	}

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Voxel3D", "", {"v3a", "v3b"}, {}, FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
