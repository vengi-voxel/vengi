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
	struct Header {
		uint8_t len;
		uint8_t colorStartIdx;
		uint8_t colorEndIdx;
		uint8_t airStartIdx;
	};
	static_assert(sizeof(Header) == 4, "Unexpected size of span header struct");
	static bool isSurface(const RawVolume *v, int x, int y, int z);
	bool loadMap(const core::String& filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph, int width, int height, int depths);
public:
	bool loadGroups(const core::String& filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
