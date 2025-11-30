/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include "color/RGBA.h"
#include "io/Stream.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"

namespace voxelformat {
namespace priv {

enum class SLABVisibility : uint8_t { None = 0, Left = 1, Right = 2, Front = 4, Back = 8, Up = 16, Down = 32 };
CORE_ENUM_BIT_OPERATIONS(SLABVisibility)
SLABVisibility calculateVisibility(const voxel::RawVolume *v, int x, int y, int z);

bool readColor(io::SeekableReadStream &stream, core::RGBA &color, bool bgr, bool scale);
bool writeColor(io::SeekableWriteStream &stream, core::RGBA color, bool bgr, bool scale);

inline bool readBGRScaledColor(io::SeekableReadStream &stream, core::RGBA &color) {
	return readColor(stream, color, true, true);
}

inline bool writeBGRScaledColor(io::SeekableWriteStream &stream, core::RGBA color) {
	return writeColor(stream, color, true, true);
}

inline bool readRGBScaledColor(io::SeekableReadStream &stream, core::RGBA &color) {
	return readColor(stream, color, false, true);
}

inline bool writeRGBScaledColor(io::SeekableWriteStream &stream, core::RGBA color) {
	return writeColor(stream, color, false, true);
}

inline bool readRGBColor(io::SeekableReadStream &stream, core::RGBA &color) {
	return readColor(stream, color, false, false);
}

inline bool writeRGBColor(io::SeekableWriteStream &stream, core::RGBA color) {
	return writeColor(stream, color, false, false);
}

inline bool readBGRColor(io::SeekableReadStream &stream, core::RGBA &color) {
	return readColor(stream, color, true, false);
}

inline bool writeBGRColor(io::SeekableWriteStream &stream, core::RGBA color) {
	return writeColor(stream, color, true, false);
}

} // namespace priv
} // namespace voxelformat
