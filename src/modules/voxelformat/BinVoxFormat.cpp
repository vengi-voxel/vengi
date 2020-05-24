/**
 * @file
 */

#include "BinVoxFormat.h"
#include "voxel/MaterialColor.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include <SDL_stdinc.h>
#include <string.h>

namespace voxel {

bool BinVoxFormat::readHeader(const core::String& header) {
	const char *delimiters = "\n";
	// Skip delimiters at beginning.
	size_t lastPos = header.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	size_t pos = header.find_first_of(delimiters, lastPos);

	while (core::String::npos != pos || core::String::npos != lastPos) {
		// Found a token, add it to the vector.
		const core::String& line = header.substr(lastPos, pos - lastPos);

		if (line == "data") {
			return true;
		}
		if (line.size() > 127) {
			Log::error("Line width exceeded max allowed value");
			return false;
		}
		char buf[128];
		SDL_strlcpy(buf, line.c_str(), sizeof(buf));

		if (core::string::startsWith(line, "#binvox")) {
			if (SDL_sscanf(buf, "#binvox %u", &_version) != 1) {
				Log::error("Failed to parse binvox version");
				return false;
			}
		} else if (core::string::startsWith(buf, "dim ")) {
			if (SDL_sscanf(buf, "dim %u %u %u", &_d, &_h, &_w) != 3) {
				Log::error("Failed to parse binvox dimensions");
				return false;
			}
		} else if (core::string::startsWith(buf, "translate ")) {
			float tx, ty, tz;
			if (SDL_sscanf(buf, "translate %f %f %f", &tz, &ty, &tx) != 3) {
				Log::error("Failed to parse binvox translation");
				return false;
			}
			_tx = -tx;
			_ty = -ty;
			_tz = -tz;
		} else if (core::string::startsWith(buf, "scale ")) {
			if (SDL_sscanf(buf, "scale %f", &_scale) != 1) {
				Log::error("Failed to parse binvox scale");
				return false;
			}
		}

		// Skip delimiters. Note the "not_of"
		lastPos = header.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = header.find_first_of(delimiters, lastPos);
	}
	if (_version != 1) {
		Log::error("Failed to parse the header data");
		return false;
	}
	if (_w >= MaxRegionSize || _h >= MaxRegionSize || _d >= MaxRegionSize) {
		Log::error("Max region size exceeded: %u:%u:%u", _w, _h, _d);
		return false;
	}
	if (_w == 0 || _h == 0 || _d == 0) {
		Log::error("Region size invalid: %u:%u:%u", _w, _h, _d);
		return false;
	}
	_size = _w * _h * _d;
	return true;
}

/**
 * The y-coordinate runs fastest, then the z-coordinate, then the x-coordinate.
 */
static inline int get_index(int x, int y, int z, int width, int height) {
	const int index = x * (width * height) + z * width + y;
	return index;
}

static inline void get_coords(int idx, int &x, int &y, int &z, int width, int height) {
	x = idx / (width * height);
	idx -= (x * width * height);
	z = idx / width;
	y = idx % width;
}

bool BinVoxFormat::readData(const io::FilePtr& file, const size_t offset, VoxelVolumes& volumes) {
	const voxel::Region region(_tx, _tz, _ty, _tx + _w - 1, _tz + _h - 1, _ty + _d - 1);
	if (!region.isValid()) {
		Log::error("Invalid region found in file");
		return false;
	}
	core_assert(region.getWidthInVoxels() == (int32_t)_w);
	core_assert(region.getHeightInVoxels() == (int32_t)_h);
	core_assert(region.getDepthInVoxels() == (int32_t)_d);

	uint8_t *buf;
	const int fileSize = file->read((void**) &buf);
	if (fileSize <= 0) {
		Log::error("Could not read file");
		return false;
	}
	const int dataSize = fileSize - offset;
	if ((dataSize % 2) != 0) {
		Log::error("Unexpected data segment size: %i", dataSize);
		delete[] buf;
		return false;
	}

	const uint32_t bufSize = _w * _h * _d;
	uint8_t* voxelBuf = new uint8_t[bufSize]();

	uint32_t index = 0;
	uint32_t n = 0;
	for (int i = offset; i < fileSize; i += 2) {
		const uint8_t value = buf[i + 0];
		const uint8_t count = buf[i + 1];
		if (count == 0) {
			Log::error("Found invalid rle encoding");
			delete[] voxelBuf;
			delete[] buf;
			return false;
		}
		n += count;
		const uint32_t endIndex = index + count;
		if (endIndex > bufSize) {
			Log::error("The end index %i is bigger than the data size %i", endIndex, dataSize);
			delete[] voxelBuf;
			delete[] buf;
			return false;
		}
		if (value != 0u) {
			for (uint32_t v = index; v < endIndex; ++v) {
				voxelBuf[v] = value;
			}
		}
		index = endIndex;
	}
	if (n != bufSize) {
		Log::error("Unexpected voxel amount: %i, expected: %i (w: %u, h: %u, d: %u), fileSize %i, offset %i",
				dataSize, bufSize, _w, _h, _d, fileSize, (int)offset);
		return false;
	}

	const int lowerIdx = get_index(region.getLowerX(), region.getLowerY(), region.getLowerZ(),
			region.getWidthInVoxels(), region.getHeightInVoxels());
	RawVolume *volume = new RawVolume(region);
	volumes.push_back(VoxelVolume{volume, file->fileName(), true});
	for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
		for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
			for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
				const int idx = get_index(x, y, z, region.getWidthInVoxels(), region.getHeightInVoxels()) - lowerIdx;
				if (voxelBuf[idx] != 0u) {
					volume->setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, voxelBuf[idx]));
				}
			}
		}
	}
	delete[] buf;
	delete[] voxelBuf;
	return true;
}

bool BinVoxFormat::loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) {
	core::String str = file->load();
	const size_t dataOffset = str.find("data");
	if (dataOffset == core::String::npos) {
		Log::error("Could not find end of header in %s", file->name().c_str());
		return false;
	}
	if (dataOffset > 128) {
		Log::error("Max allowed header size exceeded: %i", (int)dataOffset);
		return false;
	}
	const core::String& header = str.substr(0, dataOffset);
	if (!readHeader(header)) {
		Log::error("Could not read header of %s", file->name().c_str());
		return false;
	}
	if (!readData(file, dataOffset + 5, volumes)) {
		Log::warn("Could not load the whole data from %s", file->name().c_str());
		return false;
	}
	return true;
}

bool BinVoxFormat::saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) {
	io::FileStream stream(file.get());

	RawVolume* mergedVolume = merge(volumes);
	if (mergedVolume == nullptr) {
		Log::error("Failed to merge volumes");
		return false;
	}

	const voxel::Region& region = mergedVolume->region();
	RawVolume::Sampler sampler(mergedVolume);

	const uint32_t width = region.getWidthInVoxels();
	const uint32_t height = region.getHeightInVoxels();
	const uint32_t depth = region.getDepthInVoxels();
	const glm::ivec3& offset = -region.getLowerCorner();
	const float scale = 1.0f;

	stream.addString("#binvox 1\n", false);
	stream.addStringFormat(false, "dim %u %u %u\n", width, depth, height);
	stream.addStringFormat(false, "translate %i %i %i\n", offset.x, offset.z, offset.y);
	stream.addStringFormat(false, "scale %f\n", scale);
	stream.addString("data\n", false);

	uint8_t count = 0u;
	uint8_t value = 0u;
	uint32_t voxels = 0u;
	const int maxIndex = width * height * depth;
	for (int idx = 0; idx < maxIndex; ++idx) {
		int x, y, z;
		get_coords(idx, x, y, z, width, height);
		core_assert_always(sampler.setPosition(x, z, y));
		const voxel::Voxel voxel = sampler.voxel();
		if (voxel.getMaterial() == VoxelType::Air) {
			if (value != 0u || count == 255u) {
				if (count > 0u) {
					stream.addByte(value);
					stream.addByte(count);
				}
				voxels += count;
				count = 0u;
			}
			++count;
			value = 0u;
		} else {
			const uint8_t v = voxel.getColor();
			if (value != v || count == 255u) {
				if (count > 0u) {
					stream.addByte(value);
					stream.addByte(count);
				}
				voxels += count;
				count = 0u;
			}
			++count;
			value = v;
		}
	}
	core_assert_msg(count > 0u, "Expected to have at least one voxel left: %i", (int)count);
	stream.addByte(value);
	stream.addByte(count);
	voxels += count;
	delete mergedVolume;
	const uint32_t expectedVoxels = width * height * depth;
	if (voxels != expectedVoxels) {
		Log::error("Not enough data was written: %u vs %u (w: %u, h: %u, d: %u)",
			voxels, expectedVoxels, width, height, depth);
		return false;
	}
	return true;
}

}
