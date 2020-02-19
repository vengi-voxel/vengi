/**
 * @file
 */

#include "VXMFormat.h"
#include "core/Common.h"
#include "core/Color.h"
#include "core/GLM.h"
#include "voxel/MaterialColor.h"
#include "core/Log.h"
#include <glm/common.hpp>

namespace voxel {

#define wrap(read) \
	if (read != 0) { \
		Log::error("Could not load vmx file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left (line %i)", (int)stream.remaining(), (int)__LINE__); \
		return false; \
	}

#define wrapBool(read) \
	if (read != true) { \
		Log::error("Could not load vmx file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left (line %i)", (int)stream.remaining(), (int)__LINE__); \
		return false; \
	}

bool VXMFormat::saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) {
	return false;
}

bool VXMFormat::loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) {
	if (!(bool)file || !file->exists()) {
		Log::error("Could not load vmx file: File doesn't exist");
		return false;
	}
	io::FileStream stream(file.get());

	uint32_t header;
	wrap(stream.readInt(header))
	constexpr uint32_t headerMagic5 = FourCC('V','X','M','5');
	constexpr uint32_t headerMagic4 = FourCC('V','X','M','4');
	if (header != headerMagic5 && header != headerMagic4) {
		uint8_t buf[4];
		FourCCRev(buf, header);
		Log::error("Could not load vxm file: Invalid magic found (%s)", (const char *)buf);
		return false;
	}

	bool foundPivot = false;
	glm::ivec3 ipivot { 0 };
	if (header == headerMagic4) {
		Log::debug("Found vxm4");
	} else if (header == headerMagic5) {
		Log::debug("Found vxm5");
		glm::vec3 pivot;
		wrap(stream.readFloat(pivot.x));
		wrap(stream.readFloat(pivot.y));
		wrap(stream.readFloat(pivot.z));
		ipivot.x = pivot.x;
		ipivot.y = pivot.y;
		ipivot.z = pivot.z;
		foundPivot = true;
	}

	glm::uvec2 textureDim;
	wrap(stream.readInt(textureDim.x));
	wrap(stream.readInt(textureDim.y));
	if (glm::any(glm::greaterThan(textureDim, glm::uvec2(2048)))) {
		Log::warn("Size of texture exceeds the max allowed value");
		return false;
	}

	uint32_t texAmount;
	wrap(stream.readInt(texAmount));
	if (texAmount > 0xFFFF) {
		Log::warn("Size of textures exceeds the max allowed value: %i", texAmount);
		return false;
	}

	Log::debug("texAmount: %i", (int)texAmount);
	for (uint32_t t = 0u; t < texAmount; t++) {
		char textureId[1024];
		wrapBool(stream.readString(sizeof(textureId), textureId, true));
		Log::debug("tex: %i: %s", (int)t, textureId);
		uint32_t px = 0u;
		for (;;) {
			uint8_t rleStride;
			wrap(stream.readByte(rleStride));
			if (rleStride == 0u) {
				break;
			}

			struct TexColor {
				glm::u8vec3 rgb;
			};
			static_assert(sizeof(TexColor) == 3, "Unexpected TexColor size");
			stream.skip(sizeof(TexColor));
			px += rleStride;
			if (px > textureDim.x * textureDim.y * sizeof(TexColor)) {
				Log::error("RLE texture chunk exceeds max allowed size");
			}
		}
	}

	for (int i = 0; i < 6; ++i) {
		uint32_t quadAmount;
		wrap(stream.readInt(quadAmount));
		if (quadAmount > 0x40000U) {
			Log::warn("Size of quads exceeds the max allowed value");
			return false;
		}
		struct QuadVertex {
			glm::vec3 pos;
			glm::ivec2 uv;
		};
		static_assert(sizeof(QuadVertex) == 20, "Unexpected QuadVertex size");
		stream.skip(quadAmount * 4 * sizeof(QuadVertex));
	}

	glm::uvec3 size;
	wrap(stream.readInt(size.x));
	wrap(stream.readInt(size.y));
	wrap(stream.readInt(size.z));

	if (glm::any(glm::greaterThan(size, glm::uvec3(2048)))) {
		Log::warn("Size of volume exceeds the max allowed value");
		return false;
	}
	if (glm::any(glm::lessThan(size, glm::uvec3(1)))) {
		Log::warn("Size of volume results in empty space");
		return false;
	}

	Log::debug("Volume of size %u:%u:%u", size.x, size.y, size.z);

	uint8_t materialAmount;
	wrap(stream.readByte(materialAmount));
	Log::debug("Palette of size %i", (int)materialAmount);

	uint8_t *palette = new uint8_t[materialAmount];
	for (int i = 0; i < (int) materialAmount; ++i) {
		uint8_t blue;
		wrap(stream.readByte(blue));
		uint8_t green;
		wrap(stream.readByte(green));
		uint8_t red;
		wrap(stream.readByte(red));
		uint8_t alpha;
		wrap(stream.readByte(alpha));
		uint8_t emissive;
		wrap(stream.readByte(emissive));
		const glm::vec4& rgbaColor = core::Color::fromRGBA(red, green, blue, alpha);
		palette[i] = findClosestIndex(rgbaColor);
	}

	const Region region(glm::ivec3(0), glm::ivec3(size) - 1);
	RawVolume* volume = new RawVolume(region);

	if (!foundPivot) {
		ipivot = region.getCentre();
	}

	int idx = 0;
	for (;;) {
		uint8_t length;
		wrap(stream.readByte(length));
		if (length == 0u) {
			break;
		}

		uint8_t matIdx;
		wrap(stream.readByte(matIdx));
		if (matIdx == 0xFFU) {
			idx += length;
			continue;
		}
		if (matIdx >= materialAmount) {
			// at least try to load the rest
			idx += length;
			continue;
		}

		// left to right, bottom to top, front to back
		for (int i = idx; i < idx + length; i++) {
			const int xx = i / (size.y * size.z);
			const int yy = (i / size.z) % size.y;
			const int zz = i % size.z;
			const uint8_t index = palette[matIdx];
			voxel::VoxelType voxelType = voxel::VoxelType::Generic;
			if (index == 0) {
				voxelType = voxel::VoxelType::Air;
			}
			const Voxel voxel = createColorVoxel(voxelType, index);
			volume->setVoxel(size.x - 1 - xx, yy, zz, voxel);
		}
		idx += length;
	}
	delete[] palette;
	volumes.push_back(VoxelVolume(volume, "", true, ipivot));
	return true;
}

#undef wrap
#undef wrapBool

}
