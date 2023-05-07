/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief Chronovox Studio Model and Nick's Voxel Model
 *
 * @ingroup Formats
 */
class CSMFormat : public RGBAFormat {
protected:
	bool loadGroupsRGBA(const core::String &filename, io::SeekableReadStream &stream,
						scenegraph::SceneGraph &sceneGraph, const voxel::Palette &palette,
						const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override;
};

} // namespace voxelformat
