/**
 * @file
 */

#pragma once

#include "VXLShared.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"

namespace voxelformat {

/**
 * @brief Hierarchical Voxel Animation
 *
 * @ingroup Formats
 */
class HVAFormat {
protected:
	bool writeHVAHeader(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph) const;
	bool writeHVAFrames(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph) const;
	bool readHVAHeader(io::SeekableReadStream &stream, vxl::HVAHeader &header) const;
	bool readHVAFrames(io::SeekableReadStream &stream, const vxl::VXLModel &mdl, vxl::HVAModel &file) const;

public:
	bool loadHVA(const core::String &filename, const io::ArchivePtr &archive, const vxl::VXLModel &mdl, scenegraph::SceneGraph &sceneGraph);
	bool saveHVA(const core::String &filename, const io::ArchivePtr &archive, const scenegraph::SceneGraph &sceneGraph);
};

} // namespace voxelformat
