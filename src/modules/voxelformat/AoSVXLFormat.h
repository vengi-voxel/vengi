/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxel {

/**
 * @brief AceOfSpades VXL format
 *
 * https://silverspaceship.com/aosmap/
 */
class AoSVXLFormat : public Format {
private:
	bool load(const core::String& filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph, int width, int height, int depths);
public:
	bool loadGroups(const core::String& filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
