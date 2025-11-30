/**
 * @file
 */

#include "SLABShared.h"
#include "core/Log.h"
#include "color/RGBA.h"
#include "io/Stream.h"

namespace voxelformat {
namespace priv {

SLABVisibility calculateVisibility(const voxel::RawVolume *v, int x, int y, int z) {
	SLABVisibility vis = SLABVisibility::None;
	voxel::FaceBits visBits = voxel::visibleFaces(*v, x, y, z);
	if (visBits == voxel::FaceBits::None) {
		return vis;
	}
	// x
	if ((visBits & voxel::FaceBits::NegativeX) != voxel::FaceBits::None) {
		vis |= SLABVisibility::Left;
	}
	if ((visBits & voxel::FaceBits::PositiveX) != voxel::FaceBits::None) {
		vis |= SLABVisibility::Right;
	}
	// y (our z)
	if ((visBits & voxel::FaceBits::NegativeZ) != voxel::FaceBits::None) {
		vis |= SLABVisibility::Front;
	}
	if ((visBits & voxel::FaceBits::PositiveZ) != voxel::FaceBits::None) {
		vis |= SLABVisibility::Back;
	}
	// z (our y) is running from top to bottom
	if ((visBits & voxel::FaceBits::NegativeY) != voxel::FaceBits::None) {
		vis |= SLABVisibility::Down;
	}
	if ((visBits & voxel::FaceBits::PositiveY) != voxel::FaceBits::None) {
		vis |= SLABVisibility::Up;
	}
	return vis;
}

bool readColor(io::SeekableReadStream &stream, color::RGBA &color, bool bgr, bool scale) {
	uint8_t v1, v2, v3;
	if ((stream.readUInt8(v1)) != 0) {
		Log::error("Failed to read color");
		return false;
	}
	if ((stream.readUInt8(v2)) != 0) {
		Log::error("Failed to read color");
		return false;
	}
	if ((stream.readUInt8(v3)) != 0) {
		Log::error("Failed to read color");
		return false;
	}

	if (scale) {
		const float fv1 = ((float)v1 / 63.0f * 255.0f);
		const float fv2 = ((float)v2 / 63.0f * 255.0f);
		const float fv3 = ((float)v3 / 63.0f * 255.0f);
		v1 = (uint8_t)fv1;
		v2 = (uint8_t)fv2;
		v3 = (uint8_t)fv3;
	}

	if (bgr) {
		color = color::RGBA(v3, v2, v1);
	} else {
		color = color::RGBA(v1, v2, v3);
	}
	return true;
}

bool writeColor(io::SeekableWriteStream &stream, color::RGBA color, bool bgr, bool scale) {
	uint8_t v1, v2, v3;
	if (bgr) {
		v1 = color.b;
		v2 = color.g;
		v3 = color.r;
	} else {
		v1 = color.r;
		v2 = color.g;
		v3 = color.b;
	}

	if (scale) {
		v1 = (uint8_t)((float)v1 * 63.0f / 255.0f);
		v2 = (uint8_t)((float)v2 * 63.0f / 255.0f);
		v3 = (uint8_t)((float)v3 * 63.0f / 255.0f);
	}

	if (!stream.writeUInt8(v1)) {
		Log::error("Could not write color value");
		return false;
	}
	if (!stream.writeUInt8(v2)) {
		Log::error("Could not write color value");
		return false;
	}
	if (!stream.writeUInt8(v3)) {
		Log::error("Could not write color value");
		return false;
	}
	return true;
}

} // namespace priv
} // namespace voxelformat
