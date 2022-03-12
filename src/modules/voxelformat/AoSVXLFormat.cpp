/**
 * @file
 */

#include "AoSVXLFormat.h"
#include "core/Assert.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/Map.h"
#include "core/Log.h"
#include "core/Color.h"
#include "voxel/MaterialColor.h"
#include "voxelutil/VolumeResizer.h"
#include <SDL_stdinc.h>
#include <string.h>

namespace voxel {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load AoE vxl file: Not enough data in stream " CORE_STRINGIFY(read)); \
		delete volume; \
		return false; \
	}

glm::ivec3 AoSVXLFormat::dimensions(io::SeekableReadStream &stream) const {
	int64_t initial = stream.pos();

	glm::ivec3 size(0);

	while (stream.remaining() >= (int64_t)sizeof(Header)) {
		Header header;
		stream.readUInt8(header.len);
		stream.readUInt8(header.colorStartIdx);
		stream.readUInt8(header.colorEndIdx);
		stream.readUInt8(header.airStartIdx);
		if (header.colorEndIdx + 1 > size.y) {
			size.y = header.colorEndIdx + 1;
		}
		const int64_t spanBytes = header.len > 0 ? header.len * (int)sizeof(uint32_t) : core_max(0, (header.colorEndIdx + 2 - header.colorStartIdx)) * (int)sizeof(uint32_t);
		core_assert_msg(spanBytes >= 0, "spanbytes: %i, stream pos: %i", (int)spanBytes, (int)stream.pos());
		stream.skip(spanBytes);
	}
	size.y = 1 << (int)glm::ceil(glm::log2((float)size.y));
	size.x = size.z = 512;

	stream.seek(initial);
	return size;
}

bool AoSVXLFormat::loadGroups(const core::String& filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	const glm::ivec3 size = dimensions(stream);
	return loadMap(filename, stream, sceneGraph, size.x, size.y, size.z);
}

bool AoSVXLFormat::loadMap(const core::String& filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph, int width, int height, int depths) {
	const voxel::Region region(0, 0, 0, width - 1, height - 1, depths - 1);
	const int flipHeight = height - 1;
	core_assert(region.isValid());
	RawVolume *volume = new RawVolume(region);

	core::Map<uint32_t, int, 521> paletteMap(32768);
	for (int z = 0; z < depths; ++z) {
		for (int x = 0; x < width; ++x) {
			int y = 0;
			while (!stream.eos()) {
				const int64_t cpos = stream.pos();
				Header header;
				wrap(stream.readUInt8(header.len))
				wrap(stream.readUInt8(header.colorStartIdx))
				wrap(stream.readUInt8(header.colorEndIdx))
				wrap(stream.readUInt8(header.airStartIdx))
				int paletteIndex = 1;
				if ((int)header.colorStartIdx >= height) {
					Log::error("depth (top start %i exceeds the max allowed value of %i", header.colorStartIdx, height);
					delete volume;
					return false;
				}
				if (header.colorEndIdx >= height) {
					Log::error("depth (top end %i) exceeds the max allowed value of %i", header.colorEndIdx, height);
					delete volume;
					return false;
				}
				for (y = header.colorStartIdx; y <= header.colorEndIdx; ++y) {
					uint8_t b, g, r, a;
					stream.readUInt8(b);
					stream.readUInt8(g);
					stream.readUInt8(r);
					stream.readUInt8(a); // not really alpha - some shading data
					const uint32_t rgba = core::Color::getRGBA(r, g, b);
					if (!paletteMap.get(rgba, paletteIndex)) {
						paletteIndex = findClosestIndex(rgba);
						if (paletteMap.size() < paletteMap.capacity()) {
							paletteMap.put(rgba, paletteIndex);
						}
					}
					volume->setVoxel(x, flipHeight - y, z, voxel::createVoxel(voxel::VoxelType::Generic, paletteIndex));
				}
				for (int i = y; i < height; ++i) {
					volume->setVoxel(x, flipHeight - i, z, voxel::createVoxel(voxel::VoxelType::Generic, paletteIndex));
				}
				const int lenBottom = header.colorEndIdx - header.colorStartIdx + 1;

				// check for end of data marker
				if (header.len == 0) {
					if (stream.seek(cpos + (int64_t)(sizeof(uint32_t) * (lenBottom + 1))) == -1) {
						Log::error("failed to skip");
						delete volume;
						return false;
					}
					break;
				}

				int64_t rgbaPos = stream.pos();
				// infer the number of bottom colors in next span from chunk length
				const int len_top = (header.len - 1) - lenBottom;

				if (stream.seek(cpos + (int64_t)(header.len * sizeof(uint32_t))) == -1) {
					Log::error("failed to seek");
					delete volume;
					return false;
				}
				uint8_t bottomColorEnd = 0;
				if (stream.skip(3) == -1) {
					Log::error("failed to skip");
					return false;
				}
				wrap(stream.readUInt8(bottomColorEnd))
				if (stream.seek(-4, SEEK_CUR) == -1) {
					Log::error("failed to seek");
					delete volume;
					return false;
				}

				// aka air start - exclusive
				const int bottomColorStart = bottomColorEnd - len_top;
				if (bottomColorStart < 0 || bottomColorStart >= height) {
					Log::error("depth (bottom start %i) exceeds the max allowed value of %i", bottomColorStart, height);
					delete volume;
					return false;
				}
				if (bottomColorEnd >= height) {
					Log::error("depth (bottom end %i) exceeds the max allowed value of %i", bottomColorEnd, height);
					delete volume;
					return false;
				}

				if (stream.seek(rgbaPos) == -1) {
					Log::error("failed to seek");
					delete volume;
					return false;
				}
				for (y = bottomColorStart; y < bottomColorEnd; ++y) {
					uint8_t b, g, r, a;
					stream.readUInt8(b);
					stream.readUInt8(g);
					stream.readUInt8(r);
					stream.readUInt8(a); // not really alpha - some shading data
					const uint32_t rgba = core::Color::getRGBA(r, g, b);
					if (!paletteMap.get(rgba, paletteIndex)) {
						paletteIndex = findClosestIndex(rgba);
						if (paletteMap.size() < paletteMap.capacity()) {
							paletteMap.put(rgba, paletteIndex);
						}
					}
					volume->setVoxel(x, flipHeight - y, z, voxel::createVoxel(voxel::VoxelType::Generic, paletteIndex));
				}
				if (stream.seek(cpos + (int64_t)(header.len * sizeof(uint32_t))) == -1) {
					Log::error("failed to seek");
					delete volume;
					return false;
				}
			}
		}
	}
	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(filename);
	sceneGraph.emplace(core::move(node));
	return true;
}

size_t AoSVXLFormat::loadPalette(const core::String &filename, io::SeekableReadStream& stream, Palette &palette) {
	const glm::ivec3 size = dimensions(stream);
	for (int z = 0; z < size.z; ++z) {
		for (int x = 0; x < size.x; ++x) {
			int y = 0;
			while (!stream.eos()) {
				const int64_t cpos = stream.pos();
				Header header;
				stream.readUInt8(header.len);
				stream.readUInt8(header.colorStartIdx);
				stream.readUInt8(header.colorEndIdx);
				stream.readUInt8(header.airStartIdx);
				if ((int)header.colorStartIdx >= size.x) {
					Log::error("depth (top start %i exceeds the max allowed value of %i", header.colorStartIdx, size.y);
					return palette.colorCount;
				}
				if (header.colorEndIdx >= size.y) {
					Log::error("depth (top end %i) exceeds the max allowed value of %i", header.colorEndIdx, size.y);
					return palette.colorCount;
				}
				for (y = header.colorStartIdx; y <= header.colorEndIdx; ++y) {
					uint8_t b, g, r, a;
					stream.readUInt8(b);
					stream.readUInt8(g);
					stream.readUInt8(r);
					stream.readUInt8(a); // not really alpha - some shading data
					palette.addColorToPalette(core::Color::getRGBA(r, g, b));
				}
				const int lenBottom = header.colorEndIdx - header.colorStartIdx + 1;

				// check for end of data marker
				if (header.len == 0) {
					if (stream.seek(cpos + (int64_t)(sizeof(uint32_t) * (lenBottom + 1))) == -1) {
						Log::error("failed to skip");
						return palette.colorCount;
					}
					break;
				}

				int64_t rgbaPos = stream.pos();
				// infer the number of bottom colors in next span from chunk length
				const int len_top = (header.len - 1) - lenBottom;

				if (stream.seek(cpos + (int64_t)(header.len * sizeof(uint32_t))) == -1) {
					Log::error("failed to seek");
					return palette.colorCount;
				}
				uint8_t bottomColorEnd = 0;
				if (stream.skip(3) == -1) {
					Log::error("failed to skip");
					return palette.colorCount;
				}
				stream.readUInt8(bottomColorEnd);
				if (stream.seek(-4, SEEK_CUR) == -1) {
					Log::error("failed to seek");
					return palette.colorCount;
				}

				// aka air start - exclusive
				const int bottomColorStart = bottomColorEnd - len_top;
				if (bottomColorStart < 0 || bottomColorStart >= size.y) {
					Log::error("depth (bottom start %i) exceeds the max allowed value of %i", bottomColorStart, size.y);
					return palette.colorCount;
				}
				if (bottomColorEnd >= size.y) {
					Log::error("depth (bottom end %i) exceeds the max allowed value of %i", bottomColorEnd, size.y);
					return palette.colorCount;
				}

				if (stream.seek(rgbaPos) == -1) {
					Log::error("failed to seek");
					return palette.colorCount;
				}
				for (y = bottomColorStart; y < bottomColorEnd; ++y) {
					uint8_t b, g, r, a;
					stream.readUInt8(b);
					stream.readUInt8(g);
					stream.readUInt8(r);
					stream.readUInt8(a); // not really alpha - some shading data
					palette.addColorToPalette(core::Color::getRGBA(r, g, b));
				}
				if (stream.seek(cpos + (int64_t)(header.len * sizeof(uint32_t))) == -1) {
					Log::error("failed to seek");
					return palette.colorCount;
				}
			}
		}
	}
	return palette.colorCount;
}

bool AoSVXLFormat::isSurface(const RawVolume *v, int x, int y, int z) {
	const int width = v->width();
	const int depth = v->depth();
	const int height = v->height();

	if (voxel::isAir(v->voxel(x, y, z).getMaterial()))
		return false;
	if (x > 0 && voxel::isAir(v->voxel(x - 1, y, z).getMaterial()))
		return true;
	if (x + 1 < width && voxel::isAir(v->voxel(x + 1, y, z).getMaterial()))
		return true;
	if (z > 0 && voxel::isAir(v->voxel(x, y, z - 1).getMaterial()))
		return true;
	if (z + 1 < depth && voxel::isAir(v->voxel(x, y, z + 1).getMaterial()))
		return true;
	if (y > 0 && voxel::isAir(v->voxel(x, y - 1, z).getMaterial()))
		return true;
	if (y + 1 < height && voxel::isAir(v->voxel(x, y + 1, z).getMaterial()))
		return true;
	return false;
}

// code taken from https://silverspaceship.com/aosmap/aos_file_format.html
bool AoSVXLFormat::saveGroups(const SceneGraph &sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	RawVolume* mergedVolume = merge(sceneGraph);
	glm::ivec3 size = mergedVolume->region().getDimensionsInVoxels();
	glm::ivec3 targetSize(512, size.y, 512);
	if (targetSize.y < 64) {
		targetSize.y = 64;
	} else if (targetSize.y <= 256) {
		targetSize.y = 256;
	} else {
		Log::error("Volume height exceeds the max allowed height of 256 voxels: %i", targetSize.y);
		delete mergedVolume;
		return false;
	}
	const glm::ivec3 sizeDelta = targetSize - size;
	RawVolume* v = mergedVolume;
	if (glm::any(glm::notEqual(glm::ivec3(0), sizeDelta))) {
		v = resize(mergedVolume, sizeDelta);
		delete mergedVolume;
	}
	if (v == nullptr) {
		return false;
	}
	core::ScopedPtr<RawVolume> scopedPtr(v);

	const voxel::Region& region = v->region();
	const int width = region.getWidthInVoxels();
	const int depth = region.getDepthInVoxels();
	const int height = region.getHeightInVoxels();
	const int flipHeight = height - 1;

	Log::debug("Save vxl of size %i:%i:%i", width, height, depth);

	v->translate(region.getLowerCorner());
	RawVolume::Sampler sampler(v);

	const voxel::Palette& palette = voxel::getPalette();

	for (int z = 0; z < depth; ++z) {
		for (int x = 0; x < width; ++x) {
			int ypos = 0;
			while (ypos < height) {
				sampler.setPosition(x, flipHeight - ypos, z);
				// find the air region
				int air_start = ypos;
				while (ypos < height && voxel::isAir(sampler.voxel().getMaterial())) {
					++ypos;
					sampler.moveNegativeY();
				}

				// find the top region
				int top_colors_start = ypos;
				while (ypos < height && isSurface(v, x, flipHeight - ypos, z)) {
					++ypos;
				}
				int top_colors_end = ypos; // exclusive

				sampler.setPosition(x, flipHeight - ypos, z);
				// now skip past the solid voxels
				while (ypos < height && voxel::isBlocked(sampler.voxel().getMaterial()) && !isSurface(v, x, flipHeight - ypos, z)) {
					++ypos;
					sampler.moveNegativeY();
				}

				// at the end of the solid voxels, we have colored voxels.
				// in the "normal" case they're bottom colors; but it's
				// possible to have air-color-solid-color-solid-color-air,
				// which we encode as air-color-solid-0, 0-color-solid-air

				// so figure out if we have any bottom colors at this point
				int bottom_colors_start = ypos;

				int y = ypos;
				while (y < height && isSurface(v, x, flipHeight - y, z)) {
					++y;
				}

				if (y == height || 0) {
					; // in this case, the bottom colors of this span are empty, because we'll emit as top colors
				} else {
					// otherwise, these are real bottom colors so we can write them
					while (isSurface(v, x, flipHeight - ypos, z)) {
						++ypos;
					}
				}
				int bottom_colors_end = ypos; // exclusive

				// now we're ready to write a span
				int top_colors_len = top_colors_end - top_colors_start;
				int bottom_colors_len = bottom_colors_end - bottom_colors_start;

				int colors = top_colors_len + bottom_colors_len;

				if (ypos == height) {
					stream.writeUInt8(0); // last span
				} else {
					stream.writeUInt8(colors + 1);
				}

				stream.writeUInt8(top_colors_start);
				stream.writeUInt8(top_colors_end - 1);
				stream.writeUInt8(air_start);

				for (y = 0; y < top_colors_len; ++y) {
					sampler.setPosition(x, flipHeight - (top_colors_start + y), z);
					const uint32_t color = palette.colors[sampler.voxel().getColor()];
					stream.writeUInt32(color);
				}
				for (y = 0; y < bottom_colors_len; ++y) {
					sampler.setPosition(x, flipHeight - (bottom_colors_start + y), z);
					const uint32_t color = palette.colors[sampler.voxel().getColor()];
					stream.writeUInt32(color);
				}
			}
		}
	}
	return true;
}

#undef wrap

}
