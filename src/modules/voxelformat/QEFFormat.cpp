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
		Log::error("Could not load qef file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

#define wrapBool(read) \
	if ((read) == false) { \
		Log::error("Could not load qef file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

bool QEFFormat::loadGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	char buf[64];

	wrapBool(stream.readLine(sizeof(buf), buf))
	if (SDL_strcmp(buf, "Qubicle Exchange Format") != 0) {
		Log::error("Unexpected magic line: '%s'", buf);
		return false;
	}
	wrapBool(stream.readLine(sizeof(buf), buf))
	if (SDL_strcmp(buf, "Version 0.2") != 0) {
		Log::error("Unexpected version line: '%s'", buf);
		return false;
	}
	wrapBool(stream.readLine(sizeof(buf), buf))
	if (SDL_strcmp(buf, "www.minddesk.com") != 0) {
		Log::error("Unexpected url line: '%s'", buf);
		return false;
	}

	int width, height, depth;
	wrapBool(stream.readLine(sizeof(buf), buf))
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
	wrapBool(stream.readLine(sizeof(buf), buf))
	if (SDL_sscanf(buf, "%i", &paletteSize) != 1) {
		Log::error("Failed to parse palette size");
		return false;
	}

	if (paletteSize > voxel::PaletteMaxColors) {
		Log::error("Max palette size exceeded");
		return false;
	}

	_palette.colorCount = paletteSize;

	for (int i = 0; i < paletteSize; ++i) {
		float r, g, b;
		wrapBool(stream.readLine(sizeof(buf), buf))
		if (SDL_sscanf(buf, "%f %f %f", &r, &g, &b) != 3) {
			Log::error("Failed to parse palette color");
			return false;
		}
		const glm::vec4 color(r, g, b, 1.0f);
		_palette.colors[i] = core::Color::getRGBA(color);
		const uint8_t index = findClosestIndex(_palette.colors[i]);
		_paletteMapping[i] = index;
	}
	voxel::RawVolume* volume = new voxel::RawVolume(region);
	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(filename);
	sceneGraph.emplace(core::move(node));

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

bool QEFFormat::saveGroups(const SceneGraph &sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	stream.writeString("Qubicle Exchange Format\n", false);
	stream.writeString("Version 0.2\n", false);
	stream.writeString("www.minddesk.com\n", false);

	RawVolume* mergedVolume = merge(sceneGraph);

	const voxel::Region& region = mergedVolume->region();
	RawVolume::Sampler sampler(mergedVolume);
	const glm::ivec3& lower = region.getLowerCorner();

	const uint32_t width = region.getWidthInVoxels();
	const uint32_t height = region.getHeightInVoxels();
	const uint32_t depth = region.getDepthInVoxels();
	stream.writeStringFormat(false, "%i %i %i\n", width, depth, height);
	const voxel::Palette& palette = voxel::getPalette();
	stream.writeStringFormat(false, "%i\n", palette.colorCount);
	for (int i = 0; i < palette.colorCount; ++i) {
		const uint32_t c = palette.colors[i];
		const glm::vec4 &cv = core::Color::fromRGBA(c);
		stream.writeStringFormat(false, "%f %f %f\n", cv.r, cv.g, cv.b);
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
				// if (mask &&  2 ==  2) // left side visible
				// if (mask &&  4 ==  4) // right side visible
				// if (mask &&  8 ==  8) // top side visible
				// if (mask && 16 == 16) // bottom side visible
				// if (mask && 32 == 32) // front side visible
				// if (mask && 64 == 64) // back side visible
				const int vismask = 0x7E; // TODO: this produces voxels where every side is visible, it's up to the importer to fix this atm
				stream.writeStringFormat(false, "%i %i %i %i %i\n", x, z, y, voxel.getColor(), vismask);
			}
		}
	}
	delete mergedVolume;
	return true;
}

#undef wrap
#undef wrapBool

}
