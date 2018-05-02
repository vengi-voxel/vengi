/**
 * @file
 */

#include "QBFormat.h"
#include "voxel/polyvox/VolumeMerger.h"
#include "core/Common.h"
#include "core/Zip.h"
#include "core/Color.h"

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

#define wrap(read) \
	if (read != 0) { \
		Log::error("Could not load qb file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		return nullptr; \
	}

#define wrapBool(read) \
	if (read == false) { \
		Log::error("Could not load qb file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		return nullptr; \
	}

#define wrapColor(read) \
	if (read != 0) { \
		Log::error("Could not load qb file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		return voxel::Voxel(); \
	}

#define setBit(val, index) val &= (1 << (index))

bool QBFormat::save(const RawVolume* volume, const io::FilePtr& file) {
	io::FileStream stream(file.get());
	wrapSave(stream.addInt(257))
	ColorFormat colorFormat = ColorFormat::RGBA;
	ZAxisOrientation zAxisOrientation = ZAxisOrientation::Right;
	Compression compression = Compression::RLE;
	VisibilityMask visibilityMask = VisibilityMask::AlphaChannelVisibleByValue;
	wrapSave(stream.addInt((uint32_t)colorFormat))
	wrapSave(stream.addInt((uint32_t)zAxisOrientation))
	wrapSave(stream.addInt((uint32_t)compression))
	wrapSave(stream.addInt((uint32_t)visibilityMask))
	wrapSave(stream.addInt(1))
	wrapSave(stream.addByte(0)); // no name

	const voxel::Region& region = volume->region();
	const glm::ivec3 size = region.getDimensionsInVoxels();
	wrapSave(stream.addInt(size.x));
	wrapSave(stream.addInt(size.y));
	wrapSave(stream.addInt(size.z));

	const int offset = 0;
	wrapSave(stream.addInt(offset));
	wrapSave(stream.addInt(offset));
	wrapSave(stream.addInt(offset));

	int axisIndex1;
	int axisIndex2;
	if (zAxisOrientation == ZAxisOrientation::Right) {
		axisIndex1 = 0;
		axisIndex2 = 2;
	} else {
		axisIndex1 = 2;
		axisIndex2 = 0;
	}

	constexpr voxel::Voxel Empty;
	const int32_t EmptyColor = core::Color::getRGB(getColor(Empty));

	const glm::ivec3& mins = region.getLowerCorner();
	const glm::ivec3& maxs = region.getUpperCorner();

	int32_t currentColor = EmptyColor;
	int count = 0;

	for (int axis1 = mins[axisIndex2]; axis1 <= maxs[axisIndex2]; ++axis1) {
		for (int y = maxs[1]; y >= mins[1]; --y) {
			for (int axis2 = mins[axisIndex1]; axis2 <= maxs[axisIndex1]; ++axis2) {
				int x, z;
				const bool rightHanded = zAxisOrientation == ZAxisOrientation::Right;
				if (rightHanded) {
					x = axis2;
					z = axis1;
				} else {
					x = axis1;
					z = axis2;
				}
				const Voxel& voxel = volume->voxel(x, y, z);
				Log::debug("Save voxel: x %i, y %i, z %i (color: %i)", x, y, z, (int)voxel.getColor());
				int32_t newColor;
				if (voxel == Empty) {
					newColor = EmptyColor;
				} else {
					uint8_t visible;
					if (visibilityMask == VisibilityMask::AlphaChannelVisibleSidesEncoded) {
						// TODO: this looks wrong - use the Sides enum
						voxel::RawVolume::Sampler sampler(volume);
						sampler.setPosition(x, y, z);
						visible = 0;
						if (sampler.peekVoxel0px0py1pz() == Empty) {
							setBit(visible, rightHanded ? 1 : 6);
						}
						if (sampler.peekVoxel0px0py1nz() == Empty) {
							setBit(visible, rightHanded ? 2 : 5);
						}
						if (sampler.peekVoxel0px1py0pz() == Empty) {
							setBit(visible, 3);
						}
						if (sampler.peekVoxel0px1ny0pz() == Empty) {
							setBit(visible, 4);
						}
						if (sampler.peekVoxel1nx0py0pz() == Empty) {
							setBit(visible, rightHanded ? 5 : 1);
						}
						if (sampler.peekVoxel1px0py0pz() == Empty) {
							setBit(visible, rightHanded ? 6 : 2);
						}
					} else {
						visible = 255;
					}
					const int32_t voxelColor = core::Color::getRGBA(getColor(voxel));
					const uint8_t red = (voxelColor >> 24) & 0xFF;
					const uint8_t green = (voxelColor >> 16) & 0xFF;
					const uint8_t blue = (voxelColor >> 8) & 0xFF;
					if (colorFormat == ColorFormat::RGBA) {
						newColor = ((uint32_t)red) << 24 | ((uint32_t)green) << 16 | ((uint32_t)blue) << 8 | ((uint32_t)visible) << 0;
					} else {
						newColor = ((uint32_t)blue) << 24 | ((uint32_t)green) << 16 | ((uint32_t)red) << 8 | ((uint32_t)visible) << 0;
					}
				}

				if (compression == Compression::RLE) {
					if (newColor != currentColor) {
						if (count == 1) {
							wrapSave(stream.addInt(currentColor))
						} else if (count == 2) {
							wrapSave(stream.addInt(currentColor))
							wrapSave(stream.addInt(currentColor))
						} else if (count == 3) {
							wrapSave(stream.addInt(currentColor))
							wrapSave(stream.addInt(currentColor))
							wrapSave(stream.addInt(currentColor))
						} else if (count > 3) {
							wrapSave(stream.addInt(RLE_FLAG))
							wrapSave(stream.addInt(count))
							wrapSave(stream.addInt(currentColor))
						}
						count = 0;
					}
					currentColor = newColor;
					count++;
				} else {
					wrapSave(stream.addInt(newColor))
				}
			}
		}
		if (compression == Compression::RLE) {
			if (count == 1) {
				wrapSave(stream.addInt(currentColor))
			} else if (count == 2) {
				wrapSave(stream.addInt(currentColor))
				wrapSave(stream.addInt(currentColor))
			} else if (count == 3) {
				wrapSave(stream.addInt(currentColor))
				wrapSave(stream.addInt(currentColor))
				wrapSave(stream.addInt(currentColor))
			} else if (count > 3) {
				wrapSave(stream.addInt(RLE_FLAG))
				wrapSave(stream.addInt(count))
				wrapSave(stream.addInt(currentColor))
			}
			count = 0;
			wrapSave(stream.addInt(NEXT_SLICE_FLAG));
		}
	}
	return true;
}

void QBFormat::setVoxel(voxel::RawVolume* volume, uint32_t x, uint32_t y, uint32_t z, const glm::ivec3& offset, const voxel::Voxel& voxel) {
	const int32_t fx = offset.x + x;
	const int32_t fy = offset.y + y;
	const int32_t fz = offset.z + z;
	Log::debug("Set voxel %i to %i:%i:%i", (int)voxel.getMaterial(), fx, fy, fz);
	if (_zAxisOrientation == ZAxisOrientation::Right) {
		volume->setVoxel(fx, fy, fz, voxel);
	} else {
		volume->setVoxel(fz, fy, fx, voxel);
	}
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
		color = core::Color::fromRGBA(((uint32_t)red) << 24 | ((uint32_t)green) << 16 | ((uint32_t)blue) << 8 | ((uint32_t)255) << 0);
	} else {
		color = core::Color::fromRGBA(((uint32_t)blue) << 24 | ((uint32_t)green) << 16 | ((uint32_t)red) << 8 | ((uint32_t)255) << 0);
	}
	const uint8_t index = findClosestIndex(color);
	return voxel::createVoxel(voxel::VoxelType::Generic, index);
}

voxel::RawVolume* QBFormat::loadMatrix(io::FileStream& stream) {
	char buf[260] = "";
	uint8_t nameLength;
	wrap(stream.readByte(nameLength));
	Log::debug("Matrix name length: %u", (uint32_t)nameLength);
	wrapBool(stream.readString(nameLength, buf));
	buf[nameLength] = '\0';
	Log::debug("Matrix name: %s", buf);

	glm::uvec3 size(0);
	wrap(stream.readInt(size.x));
	wrap(stream.readInt(size.y));
	wrap(stream.readInt(size.z));
	Log::debug("Matrix size: %i:%i:%i", size.x, size.y, size.z);

	if (size.x == 0 || size.y == 0 || size.z == 0) {
		Log::error("Invalid size");
		return nullptr;
	}

	glm::ivec3 offset(0);
	wrap(stream.readInt((uint32_t&)offset.x));
	wrap(stream.readInt((uint32_t&)offset.y));
	wrap(stream.readInt((uint32_t&)offset.z));
	Log::debug("Matrix offset: %i:%i:%i", offset.x, offset.y, offset.z);

	voxel::Region region;
	const glm::ivec3 maxs(offset.x + size.x, offset.y + size.y, offset.z + size.z);
	if (_zAxisOrientation == ZAxisOrientation::Right) {
		region = voxel::Region(offset.x, offset.y, offset.z, maxs.x, maxs.y, maxs.z);
	} else {
		region = voxel::Region(offset.z, offset.y, offset.x, maxs.z, maxs.y, maxs.x);
	}
	core_assert(region.getDimensionsInCells() == glm::ivec3(size));
	if (!region.isValid()) {
		return nullptr;
	}
	voxel::RawVolume* volume = new voxel::RawVolume(region);
	if (_compressed == Compression::None) {
		Log::debug("qb matrix uncompressed");
		for (uint32_t z = 0; z < size.z; ++z) {
			for (uint32_t y = 0; y < size.y; ++y) {
				for (uint32_t x = 0; x < size.x; ++x) {
					const voxel::Voxel& voxel = getVoxel(stream);
					setVoxel(volume, x, y, z, offset, voxel);
				}
			}
		}
		return volume;
	}

	Log::debug("Matrix rle compressed");

	uint32_t z = 0u;
	while (z < size.z) {
		int index = -1;
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
			for (uint32_t j = 0; j < count; ++j, ++index) {
				const int x = (index + 1) % size.x;
				const int y = (index + 1) / size.x;
				setVoxel(volume, x, y, z, offset, voxel);
			}
		}
		++z;
	}
	Log::debug("Matrix read");
	return volume;
}

RawVolume* QBFormat::loadFromStream(io::FileStream& stream) {
	wrap(stream.readInt(_version))
	uint32_t colorFormat;
	wrap(stream.readInt(colorFormat))
	_colorFormat = (ColorFormat)colorFormat;
	uint32_t zAxisOrientation;
	wrap(stream.readInt(zAxisOrientation))
	_zAxisOrientation = (ZAxisOrientation)zAxisOrientation;
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

	glm::ivec3 mins(std::numeric_limits<int32_t>::max());
	glm::ivec3 maxs(std::numeric_limits<int32_t>::min());
	std::vector<voxel::RawVolume*> volumes;
	volumes.reserve(numMatrices);
	for (uint32_t i = 0; i < numMatrices; i++) {
		Log::debug("Loading matrix: %u", i);
		voxel::RawVolume* v = loadMatrix(stream);
		if (v == nullptr) {
			break;
		}
		const voxel::Region& region = v->region();
		mins = glm::min(mins, region.getLowerCorner());
		maxs = glm::max(maxs, region.getUpperCorner());
		volumes.push_back(v);
	}
	if (volumes.empty()) {
		return nullptr;
	}

	const voxel::Region mergedRegion(glm::ivec3(0), maxs - mins);
	Log::debug("Starting to merge volumes into one: %i:%i:%i - %i:%i:%i",
			mergedRegion.getLowerX(), mergedRegion.getLowerY(), mergedRegion.getLowerZ(),
			mergedRegion.getUpperX(), mergedRegion.getUpperY(), mergedRegion.getUpperZ());
	Log::debug("Mins: %i:%i:%i Maxs %i:%i:%i", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
	voxel::RawVolume* merged = new voxel::RawVolume(mergedRegion);
	const glm::ivec3& center = mergedRegion.getCentre();
	const glm::ivec3 lc(center.x, 0, center.z);
	const glm::ivec3 uc(center.x, mergedRegion.getUpperY(), center.z);
	for (voxel::RawVolume* v : volumes) {
		const voxel::Region& sr = v->region();
		const glm::ivec3& destMins = lc + sr.getLowerCorner();
		const voxel::Region dr(destMins, destMins + sr.getDimensionsInCells());
		Log::debug("Merge %i:%i:%i - %i:%i:%i into %i:%i:%i - %i:%i:%i",
				sr.getLowerX(), sr.getLowerY(), sr.getLowerZ(),
				sr.getUpperX(), sr.getUpperY(), sr.getUpperZ(),
				dr.getLowerX(), dr.getLowerY(), dr.getLowerZ(),
				dr.getUpperX(), dr.getUpperY(), dr.getUpperZ());
		voxel::mergeVolumes(merged, v, dr, sr);
		delete v;
	}
	return merged;
}

RawVolume* QBFormat::load(const io::FilePtr& file) {
	if (!(bool)file || !file->exists()) {
		Log::error("Could not load qb file: File doesn't exist");
		return nullptr;
	}
	io::FileStream stream(file.get());
	voxel::RawVolume* volume = loadFromStream(stream);
	return volume;
}

}
