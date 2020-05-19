/**
 * @file
 */

#include "BinVoxFormat.h"
#include "voxel/MaterialColor.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include <SDL.h>
#include <string.h>
#include <memory>

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
			if (SDL_sscanf(buf, "dim %u %u %u", &_w, &_h, &_d) != 3) {
				Log::error("Failed to parse binvox dimensions");
				return false;
			}
		} else if (core::string::startsWith(buf, "translate ")) {
			if (SDL_sscanf(buf, "translate %f %f %f", &_tx, &_ty, &_tz) != 3) {
				Log::error("Failed to parse binvox translation");
				return false;
			}
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

bool BinVoxFormat::readData(const io::FilePtr& file, const size_t offset, VoxelVolumes& volumes) {
	uint8_t *buf;
	const int fileSize = file->read((void**) &buf);
	std::unique_ptr<uint8_t[]> pFile(buf);

	RawVolume *volume = new RawVolume(voxel::Region(_tz, _ty, _tx, _tz + _d - 1, _ty + _h - 1, _tx + _w - 1));
	volumes.push_back(VoxelVolume{volume, file->fileName(), true});

	const int dataSize = fileSize - offset;
	const int bufSize = _w * _h * _d;
	uint8_t* voxelBuf = new uint8_t[bufSize]();
	std::unique_ptr<uint8_t[]> pVoxelBuf(voxelBuf);

	int index = 0;
	int n = 0;
	if ((dataSize % 2) != 0) {
		Log::warn("Expected data segment size %i", dataSize);
	}
	for (int i = offset; i < fileSize; i += 2) {
		const uint8_t value = buf[i + 0];
		if (value > 1) {
			Log::error("Invalid value at offset %i (currently at index: %i)", i, index);
		}
		const uint8_t count = buf[i + 1];
		n += count;
		const int endIndex = index + count;
		if (endIndex > bufSize) {
			Log::error("The end index %i is bigger than the data size %i", endIndex, dataSize);
			break;
		}
		if (value != 0) {
			for (int v = index; v < endIndex; ++v) {
				voxelBuf[v] = 1;
			}
		}
		index = endIndex;
	}
	if (n != bufSize) {
		Log::warn("Unexpected voxel amount: %i, expected: %i (w: %i, h: %i, d: %i), fileSize %i, offset %i",
				dataSize, bufSize, (int)_w, (int)_h, (int)_d, fileSize, (int)offset);
	}

	const voxel::Voxel &voxel = voxel::createRandomColorVoxel(voxel::VoxelType::Generic);
	int idx = 0;
	for (uint32_t x = _tz; x < _tz + _d; ++x) {
		for (uint32_t y = _tx; y < _tx + _w; ++y) {
			for (uint32_t z = _ty; z < _ty + _h; ++z) {
				if (voxelBuf[idx] == 1) {
					volume->setVoxel(z, y, x, voxel);
				}
				++idx;
			}
		}
	}

	return true;
}

bool BinVoxFormat::loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) {
	core::String str = file->load();
	const size_t dataOffset = str.find("data");
	if (dataOffset == core::String::npos) {
		Log::error("Could not find end of header in %s", file->name().c_str());
		return false;
	}
	const core::String& header = str.substr(0, dataOffset);
	if (!readHeader(header)) {
		Log::error("Could not read header of %s", file->name().c_str());
		return false;
	}
	if (!readData(file, dataOffset + 5, volumes)) {
		Log::warn("Could not load the whole data from %s", file->name().c_str());
	}
	return true;
}

bool BinVoxFormat::saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) {
	return false;
}

}
