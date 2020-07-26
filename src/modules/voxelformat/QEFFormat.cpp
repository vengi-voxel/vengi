/**
 * @file
 */

#include "QEFFormat.h"
#include "SDL_stdinc.h"
#include "core/Log.h"
#include "core/GLM.h"
#include "voxel/Voxel.h"
#include <glm/common.hpp>

namespace voxel {

#define wrap(read) \
	if (read != 0) { \
		Log::error("Could not load qef file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left (line %i)", (int)stream.remaining(), (int)__LINE__); \
		return false; \
	}

#define wrapBool(read) \
	if (read == false) { \
		Log::error("Could not load qef file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left (line %i)", (int)stream.remaining(), (int)__LINE__); \
		return false; \
	}

bool QEFFormat::loadGroups(const io::FilePtr &file, VoxelVolumes &volumes) {
	if (!(bool)file || !file->exists()) {
		Log::error("Could not load qef file: File doesn't exist");
		return false;
	}

	io::FileStream stream(file.get());

	char buf[64];

	wrapBool(stream.readLine(64, buf))
	if (SDL_strcmp(buf, "Qubicle Exchange Format")) {
		Log::error("Unexpected magic line: '%s'", buf);
		return false;
	}
	wrapBool(stream.readLine(64, buf))
	if (SDL_strcmp(buf, "Version 0.2")) {
		Log::error("Unexpected version line: '%s'", buf);
		return false;
	}
	wrapBool(stream.readLine(64, buf))
	if (SDL_strcmp(buf, "www.minddesk.com")) {
		Log::error("Unexpected url line: '%s'", buf);
		return false;
	}

	int width, height, depth;
	wrapBool(stream.readLine(64, buf))
	if (SDL_sscanf(buf, "%i %i %i", &width, &depth, &height) != 3) {
		Log::error("Failed to parse dimensions");
		return false;
	}

	const glm::ivec3 size(width, height, depth);
	if (glm::any(glm::greaterThan(size, glm::ivec3(MaxRegionSize)))) {
		Log::warn("Size of matrix exceeds the max allowed value");
		return false;
	}
	if (glm::any(glm::lessThan(size, glm::ivec3(1)))) {
		Log::warn("Size of matrix results in empty space");
		return false;
	}

	const voxel::Region region(glm::ivec3(0), glm::ivec3(size) - 1);
	if (!region.isValid()) {
		Log::error("Invalid region");
		return false;
	}

	int paletteSize;
	wrapBool(stream.readLine(64, buf))
	if (SDL_sscanf(buf, "%i", &paletteSize) != 1) {
		Log::error("Failed to parse palette size");
		return false;
	}

	if (paletteSize > 256) {
		Log::error("Max palette size exceeded");
		return false;
	}

	_paletteSize = paletteSize;

	for (int i = 0; i < paletteSize; ++i) {
		float r, g, b;
		wrapBool(stream.readLine(64, buf))
		if (SDL_sscanf(buf, "%f %f %f", &r, &g, &b) != 3) {
			Log::error("Failed to parse palette color");
			return false;
		}
		_palette.push_back(findClosestIndex(glm::vec4(r, g, b, 1.0f)));
	}

	voxel::RawVolume* volume = new voxel::RawVolume(region);
	volumes.push_back(VoxelVolume(volume, file->name(), true, glm::ivec3(0)));

	while (stream.remaining() > 0) {
		wrapBool(stream.readLine(64, buf))
		int x, y, z, color, vismask;
		if (SDL_sscanf(buf, "%i %i %i %i %i", &x, &z, &y, &color, &vismask) != 5) {
			Log::error("Failed to parse voxel data line");
			return false;
		}
		const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, convertPaletteIndex(color));
		volume->setVoxel(x, y, z, voxel);
	}

	return true;
}

bool QEFFormat::saveGroups(const VoxelVolumes &volumes, const io::FilePtr &file) {
	return false;
}

#undef wrap
#undef wrapBool

}
