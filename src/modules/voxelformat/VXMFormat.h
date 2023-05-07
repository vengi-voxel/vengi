/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief VoxEdit (Sandbox) (vxm)
 * The voxel model
 * @sa VXAFormat
 * @sa VXRFormat
 *
 * @ingroup Formats
 */
class VXMFormat : public PaletteFormat {
private:
	bool writeRLE(io::WriteStream &stream, int rleCount, const voxel::Voxel &voxel, const voxel::Palette &nodePalette, const voxel::Palette &palette) const;
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette, const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, const SaveContext &ctx) override;
public:
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream& stream, const LoadContext &ctx) override;
};

}
