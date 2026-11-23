/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include "color/RGBA.h"
#include "io/Stream.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

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
math::Vec3f nodeTranslation(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node);
math::Vec3f bakedNormalizedPivot(const math::Vec3f &normalizedPivot, const math::Vec3f &translation, const math::Vec3i &dims);

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
