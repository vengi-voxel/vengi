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
class QBCLFormat : public Format {
private:
	bool saveMatrix(io::SeekableWriteStream& stream, const SceneGraphNode& node) const;
	bool saveModel(io::SeekableWriteStream& stream, const SceneGraph &sceneGraph) const;

	bool readMatrix(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, const core::String &name);
	bool readModel(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, const core::String &name);
	bool readCompound(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, const core::String &name);
	bool readNodes(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent);
public:
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) override;
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
