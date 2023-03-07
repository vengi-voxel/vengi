/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief BinVox (binvox) format.
 *
 * https://www.patrickmin.com/binvox/binvox.html
 *
 * @ingroup Formats
 */
class BinVoxFormat : public NoColorFormat {
private:
	struct State {
		uint32_t _version = 0u;
		uint32_t _w = 0u;
		uint32_t _h = 0u;
		uint32_t _d = 0u;
		uint32_t _size = 0u;
		float _tx = 0.0f;
		float _ty = 0.0f;
		float _tz = 0.0f;
		float _scale = 0.0f;
	};

	bool readData(State& state, const core::String& filename, io::SeekableReadStream& stream, scenegraph::SceneGraph& sceneGraph);
protected:
	bool loadGroups(const core::String& filename, io::SeekableReadStream& stream, scenegraph::SceneGraph& sceneGraph, const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, const SaveContext &ctx) override;
};

}
