/**
 * @file
 */

#include "QEFFormat.h"
#include "voxel/MaterialColor.h"
#include "SDL_stdinc.h"
#include "core/Log.h"
#include "core/Color.h"
#include "core/GLM.h"
#include "voxel/Voxel.h"
#include <glm/common.hpp>

namespace voxel {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load qef file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left (line %i)", (int)stream.remaining(), (int)__LINE__); \
		return false; \
	}

#define wrapBool(read) \
	if ((read) == false) { \
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
	if (SDL_strcmp(buf, "Qubicle Exchange Format") != 0) {
		Log::error("Unexpected magic line: '%s'", buf);
		return false;
	}
	wrapBool(stream.readLine(64, buf))
	if (SDL_strcmp(buf, "Version 0.2") != 0) {
		Log::error("Unexpected version line: '%s'", buf);
		return false;
	}
	wrapBool(stream.readLine(64, buf))
	if (SDL_strcmp(buf, "www.minddesk.com") != 0) {
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
		_palette[i] = findClosestIndex(glm::vec4(r, g, b, 1.0f));
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
	io::FileStream stream(file.get());
	stream.addString("Qubicle Exchange Format\n", false);
	stream.addString("Version 0.2\n", false);
	stream.addString("www.minddesk.com\n", false);

	RawVolume* mergedVolume = merge(volumes);

	const voxel::Region& region = mergedVolume->region();
	RawVolume::Sampler sampler(mergedVolume);
	const glm::ivec3& lower = region.getLowerCorner();

	const MaterialColorArray& materialColors = getMaterialColors();

	const uint32_t width = region.getWidthInVoxels();
	const uint32_t height = region.getHeightInVoxels();
	const uint32_t depth = region.getDepthInVoxels();
	stream.addStringFormat(false, "%i %i %i\n", width, depth, height);
	stream.addStringFormat(false, "%i\n", (int)materialColors.size());
	for (size_t i = 0; i < materialColors.size(); ++i) {
		stream.addStringFormat(false, "%f %f %f\n", materialColors[i].r, materialColors[i].g, materialColors[i].b);
	}

	for (uint32_t x = 0u; x < width; ++x) {
		for (uint32_t y = 0u; y < height; ++y) {
			for (uint32_t z = 0u; z < depth; ++z) {
				core_assert_always(sampler.setPosition(lower.x + x, lower.y + y, lower.z + z));
				const voxel::Voxel& voxel = sampler.voxel();
				if (voxel.getMaterial() == VoxelType::Air) {
					continue;
				}
				// mask != 0 means solid, 1 is core (surrounded by others and not visible)
				const int vismask = 0xff;
				stream.addStringFormat(false, "%i %i %i %i %i\n", x, z, y, voxel.getColor(), vismask);
			}
		}
	}
	delete mergedVolume;
	return true;
}

#undef wrap
#undef wrapBool

}
