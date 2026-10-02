/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include "color/RGBA.h"
#include "io/Stream.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include <glm/vec3.hpp>

namespace scenegraph {
class SceneGraph;
class SceneGraphNode;
} // namespace scenegraph

namespace voxelformat {
namespace priv {

enum class SLABVisibility : uint8_t { None = 0, Left = 1, Right = 2, Front = 4, Back = 8, Up = 16, Down = 32 };
CORE_ENUM_BIT_OPERATIONS(SLABVisibility)
SLABVisibility calculateVisibility(const voxel::RawVolume *v, int x, int y, int z);

/**
 * KVX/KV6 only store a pivot, not a node translation. Bake the translation into a
 * normalized pivot so world placement survives save/reload (volume is always recreated
 * at (0,0,0) with an identity transform).
 */
glm::vec3 nodeTranslation(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node);
glm::vec3 bakedNormalizedPivot(const glm::vec3 &normalizedPivot, const glm::vec3 &translation, const glm::ivec3 &dims);

bool readColor(io::SeekableReadStream &stream, color::RGBA &color, bool bgr, bool scale);
bool writeColor(io::SeekableWriteStream &stream, color::RGBA color, bool bgr, bool scale);

inline bool readBGRScaledColor(io::SeekableReadStream &stream, color::RGBA &color) {
	return readColor(stream, color, true, true);
}

inline bool writeBGRScaledColor(io::SeekableWriteStream &stream, color::RGBA color) {
	return writeColor(stream, color, true, true);
}

inline bool readRGBScaledColor(io::SeekableReadStream &stream, color::RGBA &color) {
	return readColor(stream, color, false, true);
}

inline bool writeRGBScaledColor(io::SeekableWriteStream &stream, color::RGBA color) {
	return writeColor(stream, color, false, true);
}

inline bool readRGBColor(io::SeekableReadStream &stream, color::RGBA &color) {
	return readColor(stream, color, false, false);
}

inline bool writeRGBColor(io::SeekableWriteStream &stream, color::RGBA color) {
	return writeColor(stream, color, false, false);
}

inline bool readBGRColor(io::SeekableReadStream &stream, color::RGBA &color) {
	return readColor(stream, color, true, false);
}

inline bool writeBGRColor(io::SeekableWriteStream &stream, color::RGBA color) {
	return writeColor(stream, color, true, false);
}

} // namespace priv
} // namespace voxelformat
