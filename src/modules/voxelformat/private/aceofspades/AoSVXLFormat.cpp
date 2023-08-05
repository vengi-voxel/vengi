/**
 * @file
 */

#include "AoSVXLFormat.h"
#include "core/Assert.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicMap.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxel/Palette.h"
#include "voxel/PaletteLookup.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeVisitor.h"
#define libvxl_assert core_assert_msg
#define libvxl_mem_malloc core_malloc
#define libvxl_mem_realloc core_realloc
#define libvxl_mem_free core_free
extern "C" {
#include "voxelformat/external/libvxl.h"
}

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load AoE vxl file: Not enough data in stream " CORE_STRINGIFY(read));                    \
		return false;                                                                                                  \
	}

static inline uint32_t vxl_color(core::RGBA rgba) {
	return (rgba.r << 16) | (rgba.g << 8) | rgba.b;
}
static inline uint8_t vxl_blue(uint32_t c) {
	return c & 0xFF;
}
static inline uint8_t vxl_green(uint32_t c) {
	return (c >> 8) & 0xFF;
}
static inline uint8_t vxl_red(uint32_t c) {
	return (c >> 16) & 0xFF;
}

bool AoSVXLFormat::loadGroupsRGBA(const core::String &filename, io::SeekableReadStream &stream,
								  scenegraph::SceneGraph &sceneGraph, const voxel::Palette &palette,
								  const LoadContext &ctx) {
	const int64_t size = stream.size();
	uint8_t *data = (uint8_t *)core_malloc(size);
	if (stream.read(data, size) == -1) {
		Log::error("Failed to read vxl stream for %s of size %i", filename.c_str(), (int)size);
		core_free(data);
		return false;
	}

	size_t mapSize, mapHeight;
	if (!libvxl_size(&mapSize, &mapHeight, data, size)) {
		Log::error("Failed to determine vxl size");
		core_free(data);
		return false;
	}

	struct libvxl_map map;

	if (!libvxl_create(&map, mapSize, mapSize, mapHeight, data, size)) {
		Log::error("Failed to create libvxl map");
		core_free(data);
		return false;
	}

	Log::debug("Read vxl of size %i:%i:%i", (int)mapSize, (int)mapHeight, (int)mapSize);

	const voxel::Region region(0, 0, 0, (int)mapSize - 1, (int)mapHeight - 1, (int)mapSize - 1);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	scenegraph::SceneGraphNode node;
	node.setVolume(volume, true);
	voxel::PaletteLookup palLookup(palette);

	for (int x = 0; x < (int)mapSize; x++) {
		for (int y = 0; y < (int)mapSize; y++) {
			for (int z = 0; z < (int)mapHeight; z++) {
				if (!libvxl_map_issolid(&map, x, y, z)) {
					continue;
				}
				const uint32_t color = libvxl_map_get(&map, x, y, z);
				const core::RGBA rgba = core::RGBA(vxl_red(color), vxl_green(color), vxl_blue(color));
				const uint8_t paletteIndex = palLookup.findClosestIndex(rgba);
				volume->setVoxel(x, (int)mapHeight - 1 - z, y, voxel::createVoxel(palette, paletteIndex));
			}
		}
	}
	libvxl_free(&map);
	core_free(data);

	node.setName(filename);
	node.setPalette(palLookup.palette());
	sceneGraph.emplace(core::move(node));
	return true;
}

size_t AoSVXLFormat::loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
								 const LoadContext &ctx) {
	const int64_t size = stream.size();
	uint8_t *data = (uint8_t *)core_malloc(size);
	if (stream.read(data, size) == -1) {
		Log::error("Failed to read vxl stream for %s of size %i", filename.c_str(), (int)size);
		core_free(data);
		return 0;
	}

	size_t mapSize, mapHeight;
	if (!libvxl_size(&mapSize, &mapHeight, data, size)) {
		Log::error("Failed to determine vxl size");
		core_free(data);
		return 0;
	}

	Log::debug("Read vxl of size %i:%i:%i", (int)mapSize, (int)mapHeight, (int)mapSize);

	struct libvxl_map map;

	if (!libvxl_create(&map, mapSize, mapSize, mapHeight, data, size)) {
		Log::error("Failed to create libvxl map");
		core_free(data);
		return 0;
	}

	core::DynamicMap<core::RGBA, bool, 1031, core::RGBAHasher> colors;
	for (int x = 0; x < (int)mapSize; x++) {
		for (int y = 0; y < (int)mapSize; y++) {
			for (int z = 0; z < (int)mapHeight; z++) {
				if (!libvxl_map_issolid(&map, x, y, z)) {
					continue;
				}
				const uint32_t color = libvxl_map_get(&map, x, y, z);
				const core::RGBA rgba = flattenRGB(vxl_red(color), vxl_green(color), vxl_blue(color));
				colors.put(rgba, true);
			}
		}
	}
	libvxl_free(&map);
	core_free(data);

	const size_t colorCount = colors.size();
	core::Buffer<core::RGBA> colorBuffer;
	colorBuffer.reserve(colorCount);
	for (const auto &e : colors) {
		colorBuffer.push_back(e->first);
	}
	palette.quantize(colorBuffer.data(), colorBuffer.size());
	return palette.size();
}

glm::ivec3 AoSVXLFormat::maxSize() const {
	return glm::ivec3(512, 256, 512);
}

bool AoSVXLFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							  io::SeekableWriteStream &stream, const SaveContext &ctx) {
	const voxel::Region &region = sceneGraph.region();
	glm::ivec3 size = region.getDimensionsInVoxels();
	glm::ivec3 targetSize(512, size.y, 512);
	if (targetSize.y <= 64) {
		targetSize.y = 64;
	} else if (targetSize.y <= 256) {
		targetSize.y = 256;
	} else {
		Log::error("Volume height exceeds the max allowed height of 256 voxels: %i", targetSize.y);
		return false;
	}
	const int mapSize = targetSize.x;
	const int mapHeight = targetSize.y;

	Log::debug("Save vxl of size %i:%i:%i", mapSize, mapHeight, mapSize);

	struct libvxl_map map;
	if (!libvxl_create(&map, mapSize, mapSize, mapHeight, nullptr, 0)) {
		Log::error("Failed to create libvxl map");
		return false;
	}

	for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		voxelutil::visitVolume(*sceneGraph.resolveVolume(node),
							   [&map, &node, mapHeight](int x, int y, int z, const voxel::Voxel &voxel) {
								   const core::RGBA rgba = node.palette().color(voxel.getColor());
								   const uint32_t color = vxl_color(rgba);
								   libvxl_map_set(&map, x, z, mapHeight - 1 - y, color);
							   });
	}

	uint8_t buf[4096];
	struct libvxl_stream s;
	libvxl_stream(&s, &map, sizeof(buf));
	size_t read = 0;
	while ((read = libvxl_stream_read(&s, buf))) {
		if (stream.write(buf, read) == -1) {
			Log::error("Could not write AoE vxl file to stream");
			libvxl_stream_free(&s);
			libvxl_free(&map);
			return false;
		}
	}
	libvxl_stream_free(&s);
	libvxl_free(&map);
	return true;
}

#undef wrap

} // namespace voxelformat
