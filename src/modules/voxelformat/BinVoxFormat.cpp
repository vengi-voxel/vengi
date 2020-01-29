/**
 * @file
 */

#include "BinVoxFormat.h"
#include "voxel/MaterialColor.h"
#include "core/StringUtil.h"
#include <SDL.h>
#include <string.h>
#include <memory>

namespace voxel {

bool BinVoxFormat::readHeader(const std::string& header) {
	const char *delimiters = "\n";
	// Skip delimiters at beginning.
	std::string::size_type lastPos = header.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	std::string::size_type pos = header.find_first_of(delimiters, lastPos);

	while (std::string::npos != pos || std::string::npos != lastPos) {
		// Found a token, add it to the vector.
		const std::string& line = header.substr(lastPos, pos - lastPos);

		if (line == "data") {
			return true;
		}
		if (line.size() > 127) {
			Log::error("Line width exceeded max allowed value");
			return false;
		}
		char buf[128];
		strncpy(buf, line.data(), line.size());
		buf[line.size()] = '\0';

		if (core::string::startsWith(line, "#binvox")) {
			SDL_sscanf(buf, "#binvox %u", &_version);
		} else if (core::string::startsWith(buf, "dim ")) {
			SDL_sscanf(buf, "dim %u %u %u", &_w, &_h, &_d);
		} else if (core::string::startsWith(buf, "translate ")) {
			SDL_sscanf(buf, "translate %f %f %f", &_tx, &_ty, &_tz);
		} else if (core::string::startsWith(buf, "scale ")) {
			SDL_sscanf(buf, "scale %f", &_scale);
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
	_size = _w * _h * _d;
	return true;
}

bool BinVoxFormat::readData(const io::FilePtr& file, const size_t offset, VoxelVolumes& volumes) {
	uint8_t *buf;
	const int fileSize = file->read((void**) &buf);
	std::unique_ptr<uint8_t[]> pFile(buf);

	RawVolume *volume = new RawVolume(voxel::Region(0, 0, 0, _d - 1, _h - 1, _w - 1));
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
	for (uint32_t x = 0u; x < _d; ++x) {
		for (uint32_t y = 0u; y < _w; ++y) {
			for (uint32_t z = 0u; z < _h; ++z) {
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
	std::string str = file->load();
	const size_t dataOffset = str.find("data");
	if (dataOffset == std::string::npos) {
		Log::error("Could not find end of header in %s", file->name().c_str());
		return false;
	}
	const std::string& header = str.substr(0, dataOffset);
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
