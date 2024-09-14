/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

class PNGFormat : public RGBAFormat {
private:
	bool importSlices(scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
					  const io::ArchiveFiles &entities) const;
	bool importAsHeightmap(scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
						   const core::String &filename, const io::ArchivePtr &archive) const;
	bool importAsVolume(scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
						const core::String &filename, const io::ArchivePtr &archive) const;
	bool importAsPlane(scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
					   const core::String &filename, const io::ArchivePtr &archive) const;

protected:
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const palette::Palette &palette, const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;
};

} // namespace voxelformat
