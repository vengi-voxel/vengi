/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief VoxEdit (Sandbox) (vmx)
 * The voxel model
 * @sa VXAFormat
 * @sa VXRFormat
 */
class VXMFormat : public PaletteFormat {
private:
	bool writeRLE(io::WriteStream &stream, int rleCount, voxel::Voxel &voxel, uint8_t emptyColorReplacement) const;
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette) override;
public:
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
