/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief Qubicle project file (qbcl) format.
 *
 * https://gist.github.com/tostc/7f049207a2e5a7ccb714499702b5e2fd
 *
 * @see QBTFormat
 * @see QBFormat
 * @see QEFFormat
 *
 * @ingroup Formats
 */
class QBCLFormat : public RGBAFormat {
private:
	bool saveMatrix(io::SeekableWriteStream& stream, const SceneGraphNode& node) const;
	bool saveModel(io::SeekableWriteStream& stream, const SceneGraph &sceneGraph) const;

	bool readMatrix(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, const core::String &name, voxel::Palette &palette, bool paletteMode);
	bool readModel(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, const core::String &name, voxel::Palette &palette, bool paletteMode);
	bool readCompound(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, const core::String &name, voxel::Palette &palette, bool paletteMode);
	bool readNodes(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, voxel::Palette &palette, bool paletteMode);
public:
	size_t loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette) override;
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) override;
	bool loadGroupsRGBA(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, const voxel::Palette &palette) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
