/**
 * @file
 */

#include "QBFormat.h"
#include "core/Common.h"
#include "core/Zip.h"
#include "core/Color.h"
#include "core/Assert.h"

namespace voxel {

namespace {
const int RLE_FLAG = 2;
const int NEXT_SLICE_FLAG = 6;
}

#define wrapSave(write) \
	if (write == false) { \
		Log::error("Could not save qb file: " CORE_STRINGIFY(write) " failed"); \
		return false; \
	}

#define wrapSaveColor(color) \
	wrapSave(stream.addByte(color.x)) \
	wrapSave(stream.addByte(color.y)) \
	wrapSave(stream.addByte(color.z)) \
	wrapSave(stream.addByte(color.w))


#define wrap(read) \
	if (read != 0) { \
		Log::error("Could not load qb file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		return false; \
	}

#define wrapBool(read) \
	if (read == false) { \
		Log::error("Could not load qb file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		return false; \
	}

#define wrapColor(read) \
	if (read != 0) { \
		Log::error("Could not load qb file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		return voxel::Voxel(); \
	}

#define setBit(val, index) val &= (1 << (index))

bool QBFormat::saveMatrix(io::FileStream& stream, const VoxelVolume& volume) const {
	const int nameLength = volume.name.size();
	wrapSave(stream.addByte(nameLength));
	wrapSave(stream.addString(volume.name, false));

	const voxel::Region& region = volume.volume->region();
	const glm::ivec3 size = region.getDimensionsInVoxels();
	wrapSave(stream.addInt(size.x));
	wrapSave(stream.addInt(size.y));
	wrapSave(stream.addInt(size.z));

	const int offset = 0;
	wrapSave(stream.addInt(offset));
	wrapSave(stream.addInt(offset));
	wrapSave(stream.addInt(offset));

	constexpr voxel::Voxel Empty;
	const glm::ivec4 EmptyColor(0);

	const glm::ivec3& mins = region.getLowerCorner();
	const glm::ivec3& maxs = region.getUpperCorner();

	glm::ivec4 currentColor = EmptyColor;
	int count = 0;

	for (int z = mins.z; z <= maxs.z; ++z) {
		for (int y = mins.y; y <= maxs.y; ++y) {
			for (int x = mins.x; x <= maxs.x; ++x) {
				const Voxel& voxel = volume.volume->voxel(x, y, z);
				glm::ivec4 newColor;
				if (voxel == Empty) {
					newColor = EmptyColor;
					Log::debug("Save empty voxel: x %i, y %i, z %i", x, y, z);
				} else {
					const glm::vec4& voxelColor = getColor(voxel);
					const uint8_t red = voxelColor.r * 255.0f;
					const uint8_t green = voxelColor.g * 255.0f;
					const uint8_t blue = voxelColor.b * 255.0f;
					const uint8_t alpha = voxelColor.a * 255.0f;
					newColor = glm::ivec4(red, green, blue, alpha);
					Log::debug("Save voxel: x %i, y %i, z %i (color: index(%i) => rgba(%i:%i:%i:%i))",
							x, y, z, (int)voxel.getColor(), (int)red, (int)green, (int)blue, (int)alpha);
				}

				if (newColor != currentColor) {
					if (count == 1) {
						wrapSaveColor(currentColor)
					} else if (count == 2) {
						wrapSaveColor(currentColor)
						wrapSaveColor(currentColor)
					} else if (count == 3) {
						wrapSaveColor(currentColor)
						wrapSaveColor(currentColor)
						wrapSaveColor(currentColor)
					} else if (count > 3) {
						wrapSave(stream.addInt(RLE_FLAG))
						wrapSave(stream.addInt(count))
						wrapSaveColor(currentColor)
					}
					count = 0;
					currentColor = newColor;
				}
				count++;
			}
		}
		if (count == 1) {
			wrapSaveColor(currentColor)
		} else if (count == 2) {
			wrapSaveColor(currentColor)
			wrapSaveColor(currentColor)
		} else if (count == 3) {
			wrapSaveColor(currentColor)
			wrapSaveColor(currentColor)
			wrapSaveColor(currentColor)
		} else if (count > 3) {
			wrapSave(stream.addInt(RLE_FLAG))
			wrapSave(stream.addInt(count))
			wrapSaveColor(currentColor)
		}
		count = 0;
		wrapSave(stream.addInt(NEXT_SLICE_FLAG));
	}
	return true;
}

bool QBFormat::saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) {
	io::FileStream stream(file.get());
	wrapSave(stream.addInt(257))
	wrapSave(stream.addInt((uint32_t)ColorFormat::RGBA))
	wrapSave(stream.addInt((uint32_t)ZAxisOrientation::Right))
	wrapSave(stream.addInt((uint32_t)Compression::RLE))
	wrapSave(stream.addInt((uint32_t)VisibilityMask::AlphaChannelVisibleByValue))
	wrapSave(stream.addInt((int)volumes.size()))
	for (const auto& v : volumes) {
		if (!saveMatrix(stream, v)) {
			return false;
		}
	}
	return true;
}

void QBFormat::setVoxel(voxel::RawVolume* volume, uint32_t x, uint32_t y, uint32_t z, const glm::ivec3& offset, const voxel::Voxel& voxel) {
	const int32_t fx = offset.x + x;
	const int32_t fy = offset.y + y;
	const int32_t fz = offset.z + z;
	Log::debug("Set voxel %i to %i:%i:%i (z-axis: %i)", (int)voxel.getMaterial(), fx, fy, fz, (int)_zAxisOrientation);
	volume->setVoxel(fx, fy, fz, voxel);
}

voxel::Voxel QBFormat::getVoxel(io::FileStream& stream) {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
	wrapColor(stream.readByte(red))
	wrapColor(stream.readByte(green))
	wrapColor(stream.readByte(blue))
	wrapColor(stream.readByte(alpha))
	Log::debug("Red: %i, Green: %i, Blue: %i, Alpha: %i", (int)red, (int)green, (int)blue, (int)alpha);
	if (alpha == 0) {
		return voxel::Voxel();
	}
	glm::vec4 color(0.0f);
	if (_colorFormat == ColorFormat::RGBA) {
		color.r = red / 255.0f;
		color.b = blue / 255.0f;
	} else {
		color.r = blue / 255.0f;
		color.b = red / 255.0f;
	}
	color.g = green / 255.0f;
	color.a = alpha / 255.0f;
	const uint8_t index = findClosestIndex(color);
	voxel::VoxelType voxelType = voxel::VoxelType::Generic;
	if (index == 0) {
		voxelType = voxel::VoxelType::Air;
	}
	return voxel::createVoxel(voxelType, index);
}

bool QBFormat::loadMatrix(io::FileStream& stream, VoxelVolumes& volumes) {
	char name[260] = "";
	uint8_t nameLength;
	wrap(stream.readByte(nameLength));
	Log::debug("Matrix name length: %u", (uint32_t)nameLength);
	wrapBool(stream.readString(nameLength, name));
	name[nameLength] = '\0';
	Log::debug("Matrix name: %s", name);

	glm::uvec3 size(0);
	wrap(stream.readInt((uint32_t&)size.x));
	wrap(stream.readInt((uint32_t&)size.y));
	wrap(stream.readInt((uint32_t&)size.z));
	Log::debug("Matrix size: %i:%i:%i", size.x, size.y, size.z);

	if (size.x == 0 || size.y == 0 || size.z == 0) {
		Log::error("Invalid size");
		return false;
	}

	glm::ivec3 offset(0);
	wrap(stream.readInt((uint32_t&)offset.x));
	wrap(stream.readInt((uint32_t&)offset.y));
	wrap(stream.readInt((uint32_t&)offset.z));
	Log::debug("Matrix offset: %i:%i:%i", offset.x, offset.y, offset.z);

	const glm::ivec3 maxs(offset.x + size.x - 1, offset.y + size.y - 1, offset.z + size.z - 1);
	const voxel::Region region(offset.x, offset.y, offset.z, maxs.x, maxs.y, maxs.z);
	core_assert_msg(region.getDimensionsInVoxels() == glm::ivec3(size),
			"%i:%i:%i versus %i:%i:%i", region.getDimensionsInVoxels().x, region.getDimensionsInVoxels().y, region.getDimensionsInVoxels().z,
			size.x, size.y, size.z);
	if (!region.isValid()) {
		return false;
	}
	voxel::RawVolume* v = new voxel::RawVolume(region);
	volumes.push_back(VoxelVolume(v, name, true));
	if (_compressed == Compression::None) {
		Log::debug("qb matrix uncompressed");
		for (uint32_t z = 0; z < size.z; ++z) {
			for (uint32_t y = 0; y < size.y; ++y) {
				for (uint32_t x = 0; x < size.x; ++x) {
					const voxel::Voxel& voxel = getVoxel(stream);
					setVoxel(v, x, y, z, offset, voxel);
				}
			}
		}
		return true;
	}

	Log::debug("Matrix rle compressed");

	uint32_t z = 0u;
	while (z < size.z) {
		int index = 0;
		for (;;) {
			uint32_t data;
			wrap(stream.peekInt(data))
			if (data == NEXT_SLICE_FLAG) {
				stream.skip(sizeof(data));
				break;
			}

			uint32_t count = 1;
			if (data == RLE_FLAG) {
				stream.skip(sizeof(data));
				wrap(stream.readInt(count))
				Log::debug("%u voxels of the same type", count);
			}

			const voxel::Voxel& voxel = getVoxel(stream);
			for (uint32_t j = 0; j < count; ++j) {
				const int x = (index + j) % size.x;
				const int y = (index + j) / size.x;
				setVoxel(v, x, y, z, offset, voxel);
			}
			index += count;
		}
		++z;
	}
	Log::debug("Matrix read");
	return true;
}

bool QBFormat::loadFromStream(io::FileStream& stream, VoxelVolumes& volumes) {
	wrap(stream.readInt(_version))
	uint32_t colorFormat;
	wrap(stream.readInt(colorFormat))
	_colorFormat = (ColorFormat)colorFormat;
	uint32_t zAxisOrientation;
	wrap(stream.readInt(zAxisOrientation))
	_zAxisOrientation = ZAxisOrientation::Right; //(ZAxisOrientation)zAxisOrientation;
	uint32_t compressed;
	wrap(stream.readInt(compressed))
	_compressed = (Compression)compressed;
	uint32_t visibilityMaskEncoded;
	wrap(stream.readInt(visibilityMaskEncoded))
	_visibilityMaskEncoded = (VisibilityMask)visibilityMaskEncoded;

	uint32_t numMatrices;
	wrap(stream.readInt(numMatrices))

	Log::debug("Version: %u", _version);
	Log::debug("ColorFormat: %u", std::enum_value(_colorFormat));
	Log::debug("ZAxisOrientation: %u", std::enum_value(_zAxisOrientation));
	Log::debug("Compressed: %u", std::enum_value(_compressed));
	Log::debug("VisibilityMaskEncoded: %u", std::enum_value(_visibilityMaskEncoded));
	Log::debug("NumMatrices: %u", numMatrices);

	volumes.reserve(numMatrices);
	for (uint32_t i = 0; i < numMatrices; i++) {
		Log::debug("Loading matrix: %u", i);
		if (!loadMatrix(stream, volumes)) {
			break;
		}
	}
	return true;
}

bool QBFormat::loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) {
	if (!(bool)file || !file->exists()) {
		Log::error("Could not load qb file: File doesn't exist");
		return false;
	}
	io::FileStream stream(file.get());
	if (!loadFromStream(stream, volumes)) {
		return false;
	}
	return true;
}

}

#undef wrap
#undef wrapBool
#undef wrapSave
#undef wrapSaveColor
#undef wrapColor
#undef setBit
