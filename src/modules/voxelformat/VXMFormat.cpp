/**
 * @file
 */

#include "VXMFormat.h"
#include "app/App.h"
#include "io/Filesystem.h"
#include "io/FileStream.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/Color.h"
#include "core/GLM.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "io/Stream.h"
#include "voxel/MaterialColor.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "voxelformat/SceneGraphNode.h"
#include <glm/common.hpp>

namespace voxel {

static const uint8_t EMPTY_PALETTE = 0xFFu;

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load vxm file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

#define wrapDelete(read, mem) \
	if ((read) != 0) { \
		Log::error("Could not load vxm file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		delete (mem); \
		return false; \
	}

#define wrapBool(read) \
	if ((read) != true) { \
		Log::error("Could not load vxm file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

bool VXMFormat::writeRLE(io::WriteStream &stream, int length, voxel::Voxel &voxel, uint8_t emptyColorReplacement) const {
	if (length == 0) {
		return true;
	}
	wrapBool(stream.writeUInt8(length))
	if (voxel::isAir(voxel.getMaterial())) {
		wrapBool(stream.writeUInt8(EMPTY_PALETTE))
	} else if (voxel.getColor() == EMPTY_PALETTE) {
		wrapBool(stream.writeUInt8(emptyColorReplacement))
	} else {
		wrapBool(stream.writeUInt8(voxel.getColor()))
	}
	return true;
}

image::ImagePtr VXMFormat::loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) {
	const core::String imageName = filename + ".png";
	return image::loadImage(imageName, false);
}

bool VXMFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	RawVolume* mergedVolume = merge(sceneGraph);
	wrapBool(stream.writeUInt32(FourCC('V','X','M','5')));
	const glm::vec3 pivot(0.5f);
	wrapBool(stream.writeFloat(pivot.x));
	wrapBool(stream.writeFloat(pivot.y));
	wrapBool(stream.writeFloat(pivot.z));
	wrapBool(stream.writeUInt32(0)); // texture dim x
	wrapBool(stream.writeUInt32(0)); // texture dim y
	static const char *texNames[] = {"Diffuse", "Emissive"};
	const int texAmount = 0; // lengthof(texNames);
	wrapBool(stream.writeUInt32(texAmount)); // texamount

	for (int i = 0; i < texAmount; ++i) {
		stream.writeString(texNames[i], true);
		int rleCount = 0;
		uint32_t currentColor = 0;
		const int width = 0;
		const int height = 0;
		uint32_t *rgb = nullptr;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				uint32_t color = rgb[x + y * width];
				if (rleCount == 0) {
					currentColor = color;
					++rleCount;
				} else if (rleCount == 255 || currentColor != color) {
					stream.writeUInt8(rleCount);
					const glm::u8vec4& colorComp = core::Color::toRGBA(color);
					stream.writeUInt8(colorComp.r);
					stream.writeUInt8(colorComp.g);
					stream.writeUInt8(colorComp.b);
					rleCount = 1;
					currentColor = color;
				} else {
					++rleCount;
				}
			}
		}
		if (rleCount > 0) {
			stream.writeUInt8(rleCount);
			const glm::u8vec4& colorComp = core::Color::toRGBA(currentColor);
			stream.writeUInt8(colorComp.r);
			stream.writeUInt8(colorComp.g);
			stream.writeUInt8(colorComp.b);
		}
		stream.writeUInt8(0);
	}

	for (int i = 0; i < 6; ++i) {
		const int quadAmount = 0;
		wrapBool(stream.writeUInt32(quadAmount));
#if 0
		for (int i = 0; i < quadAmount; ++i) {
			for (int j = 0; j < 4; ++j) {
				stream.writeFloat(vertices[j].x);
				stream.writeFloat(vertices[j].y);
				stream.writeFloat(vertices[j].z);
				stream.writeInt32(u[j]);
				stream.writeInt32(v[j]);
			}
		}
#endif
	}

	const voxel::Palette &palette = voxel::getPalette();
	core::DynamicArray<glm::vec4> materialColors;
	palette.toVec4f(materialColors);

	core::ScopedPtr<RawVolume> scopedPtr(mergedVolume);
	const voxel::Region& region = mergedVolume->region();
	RawVolume::Sampler sampler(mergedVolume);
	const glm::ivec3& mins = region.getLowerCorner();
	const uint32_t width = region.getWidthInVoxels();
	const uint32_t height = region.getHeightInVoxels();
	const uint32_t depth = region.getDepthInVoxels();

	// we have to flip depth with height for our own coordinate system
	wrapBool(stream.writeUInt32(width))
	wrapBool(stream.writeUInt32(height))
	wrapBool(stream.writeUInt32(depth))

	// we need to find a replacement for this color - the empty voxel is the last palette entry
	// we are using the first palette entry (like magicavoxel does, too)
	const glm::vec4 emptyColor = materialColors[EMPTY_PALETTE];
	materialColors.pop();
	const uint8_t emptyColorReplacement = core::Color::getClosestMatch(emptyColor, materialColors);

	int numColors = palette.colorCount;
	if (numColors > 255) {
		numColors = 255;
	}
	if (numColors <= 0) {
		return false;
	}
	wrapBool(stream.writeUInt8(materialColors.size()))
	for (int i = 0; i < numColors; ++i) {
		const glm::u8vec4 &matcolor = core::Color::toRGBA(palette.colors[i]);
		wrapBool(stream.writeUInt8(matcolor.b))
		wrapBool(stream.writeUInt8(matcolor.g))
		wrapBool(stream.writeUInt8(matcolor.r))
		wrapBool(stream.writeUInt8(matcolor.a))
		const glm::u8vec4 &glowcolor = core::Color::toRGBA(palette.glowColors[i]);
		const bool emissive = glowcolor.a > 0;
		wrapBool(stream.writeBool(emissive))
	}
	uint32_t rleCount = 0u;
	voxel::Voxel prevVoxel;

	for (uint32_t x = 0u; x < width; ++x) {
		for (uint32_t y = 0u; y < height; ++y) {
			for (uint32_t z = 0u; z < depth; ++z) {
				core_assert_always(sampler.setPosition(mins.x + x, mins.y + y, mins.z + z));
				const voxel::Voxel &voxel = sampler.voxel();
				if (prevVoxel.getColor() != voxel.getColor() || rleCount >= 255) {
					wrapBool(writeRLE(stream, rleCount, prevVoxel, emptyColorReplacement))
					prevVoxel = voxel;
					rleCount = 0;
				}
				++rleCount;
			}
		}
	}
	if (rleCount > 0) {
		wrapBool(writeRLE(stream, rleCount, prevVoxel, emptyColorReplacement))
	}

	wrapBool(stream.writeUInt8(0))

	return true;
}

bool VXMFormat::loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) {
	uint8_t magic[4];
	wrap(stream.readUInt8(magic[0]))
	wrap(stream.readUInt8(magic[1]))
	wrap(stream.readUInt8(magic[2]))
	wrap(stream.readUInt8(magic[3]))
	if (magic[0] != 'V' || magic[1] != 'X' || magic[2] != 'M') {
		Log::error("Could not load vxm file: Invalid magic found (%c%c%c%c)",
			magic[0], magic[1], magic[2], magic[3]);
		return false;
	}
	int version;
	if (magic[3] >= '0' && magic[3] <= '9') {
		version = magic[3] - '0';
	} else if (magic[3] >= 'A' && magic[3] <= 'C') {
		version = 10 + magic[3] - 'A';
	} else {
		Log::error("Unsupported version found");
		return false;
	}

	if (version < 4 || version > 12) {
		Log::error("Could not load vxm file: Unsupported version found (%i)", version);
		return false;
	}

	SceneGraphTransform transform;
	transform.normalizedPivot = glm::vec3{0.5f, 0.0f, 0.5f};
	glm::uvec3 size(0);
	Log::debug("Found vxm%i", version);
	if (version >= 6) {
		wrap(stream.readUInt32(size.x));
		wrap(stream.readUInt32(size.y));
		wrap(stream.readUInt32(size.z));
	}
	if (version >= 5) {
		wrap(stream.readFloat(transform.normalizedPivot.x));
		wrap(stream.readFloat(transform.normalizedPivot.y));
		wrap(stream.readFloat(transform.normalizedPivot.z));
	}
	if (version >= 9) {
		uint8_t surface;
		wrap(stream.readUInt8(surface))
		if (surface) {
			uint32_t skipWidth = 0u;
			uint32_t skipHeight = 0u;
			uint32_t startx, starty, startz;
			uint32_t endx, endy, endz;
			uint32_t normal;
			// since version 10 the start and end values are floats
			// but for us this fact doesn't matter
			wrap(stream.readUInt32(startx))
			wrap(stream.readUInt32(starty))
			wrap(stream.readUInt32(startz))
			wrap(stream.readUInt32(endx))
			wrap(stream.readUInt32(endy))
			wrap(stream.readUInt32(endz))
			wrap(stream.readUInt32(normal))
			if (version >= 10) {
				wrap(stream.readUInt32(skipWidth))
				wrap(stream.readUInt32(skipHeight))
			} else {
				switch (normal) {
				case 0:
				case 1:
					skipWidth = endz - startz;
					skipHeight = endy - starty;
					break;
				case 2:
				case 3:
					skipWidth = endx - startx;
					skipHeight = endz - startz;
					break;
				case 4:
				case 5:
					skipWidth = endx - startx;
					skipHeight = endy - starty;
					break;
				}
			}
			stream.skip(skipWidth * skipHeight);
		}
	}
	if (version >= 8) {
		float dummy;                   // since version 'A'
		wrap(stream.readFloat(dummy)); // lod scale
		wrap(stream.readFloat(dummy)); // lod pivot x
		wrap(stream.readFloat(dummy)); // lod pivot y
		wrap(stream.readFloat(dummy)); // lod pivot z
	}

	uint32_t lodLevels = 1;
	if (version >= 7) {
		wrap(stream.readUInt32(lodLevels));
	}
	for (uint32_t lodLevel = 0u; lodLevel < lodLevels; ++lodLevel) {
		glm::uvec2 textureDim;
		wrap(stream.readUInt32(textureDim.x));
		wrap(stream.readUInt32(textureDim.y));
		if (glm::any(glm::greaterThan(textureDim, glm::uvec2(2048)))) {
			Log::warn("Size of texture exceeds the max allowed value");
			return false;
		}

		if (version >= 11) {
			uint32_t size;
			wrap(stream.readUInt32(size));
			stream.skip(size); // zipped pixel data
		} else {
			uint32_t texAmount;
			wrap(stream.readUInt32(texAmount));
			if (texAmount > 0xFFFF) {
				Log::warn("Size of textures exceeds the max allowed value: %i", texAmount);
				return false;
			}

			Log::debug("texAmount: %i", (int)texAmount);
			for (uint32_t t = 0u; t < texAmount; t++) {
				char textureId[1024];
				wrapBool(stream.readString(sizeof(textureId), textureId, true))
				if (version >= 6) {
					uint32_t texZipped;
					wrap(stream.readUInt32(texZipped));
					stream.skip(texZipped);
				} else {
					Log::debug("tex: %i: %s", (int)t, textureId);
					uint32_t px = 0u;
					for (;;) {
						uint8_t rleStride;
						wrap(stream.readUInt8(rleStride));
						if (rleStride == 0u) {
							break;
						}

						struct TexColor {
							glm::u8vec3 rgb;
						};
						static_assert(sizeof(TexColor) == 3, "Unexpected TexColor size");
						stream.skip(sizeof(TexColor));
						px += rleStride;
						if (px > textureDim.x * textureDim.y * sizeof(TexColor)) {
							Log::error("RLE texture chunk exceeds max allowed size");
						}
					}
				}
			}
		}

		for (int i = 0; i < 6; ++i) {
			uint32_t quadAmount;
			wrap(stream.readUInt32(quadAmount));
			if (quadAmount > 0x40000U) {
				Log::warn("Size of quads exceeds the max allowed value");
				return false;
			}
			struct QuadVertex {
				glm::vec3 pos;
				glm::ivec2 uv;
			};
			static_assert(sizeof(QuadVertex) == 20, "Unexpected QuadVertex size");
			stream.skip(quadAmount * 4 * sizeof(QuadVertex));
		}
	}

	if (version <= 5) {
		wrap(stream.readUInt32(size.x));
		wrap(stream.readUInt32(size.y));
		wrap(stream.readUInt32(size.z));
	}

	if (glm::any(glm::greaterThan(size, glm::uvec3(MaxRegionSize)))) {
		Log::warn("Size of volume exceeds the max allowed value");
		return false;
	}
	if (glm::any(glm::lessThan(size, glm::uvec3(1)))) {
		Log::warn("Size of volume results in empty space");
		return false;
	}

	Log::debug("Volume of size %u:%u:%u", size.x, size.y, size.z);

	if (version >= 11) {
		stream.skip(256l * 4l); // palette data rgba
		stream.skip(256l * 4l); // palette data rgba for emissive materials
		uint8_t chunkAmount; // palette chunks
		wrap(stream.readUInt8(chunkAmount));
		for (int i = 0; i < (int) chunkAmount; ++i) {
			char chunkId[1024];
			wrapBool(stream.readString(sizeof(chunkId), chunkId, true))
			stream.skip(1); // chunk offset
			stream.skip(1); // chunk length
		}
	}

	uint8_t materialAmount;
	wrap(stream.readUInt8(materialAmount));
	Log::debug("Palette of size %i", (int)materialAmount);
	if (materialAmount > _paletteMapping.size()) {
		Log::error("Invalid material size found: %u", materialAmount);
		return false;
	}

	for (int i = 0; i < (int) materialAmount; ++i) {
		uint8_t blue;
		wrap(stream.readUInt8(blue));
		uint8_t green;
		wrap(stream.readUInt8(green));
		uint8_t red;
		wrap(stream.readUInt8(red));
		uint8_t alpha;
		wrap(stream.readUInt8(alpha));
		uint8_t emissive;
		wrap(stream.readUInt8(emissive));
		_palette.colors[i] = core::Color::getRGBA(red, green, blue, alpha);
		_paletteMapping[i] = findClosestIndex(_palette.colors[i]);
		if (emissive) {
			_palette.setGlow(i);
		} else {
			_palette.removeGlow(i);
		}
	}
	_palette.colorCount = materialAmount;

	const Region region(glm::ivec3(0), glm::ivec3(size) - 1);

	uint8_t maxLayers = 1;
	if (version >= 12) {
		wrap(stream.readUInt8(maxLayers));
	}

	for (uint8_t layer = 0; layer < maxLayers; ++layer) {
		int idx = 0;
		bool visible = true;
		char layerName[1024];
		if (version >= 12) {
			wrapBool(stream.readString(sizeof(layerName), layerName, true))
			visible = stream.readBool();
		} else {
			core::string::formatBuf(layerName, sizeof(layerName), "Layer %i", layer);
		}
		RawVolume* volume = new RawVolume(region);
		for (;;) {
			uint8_t length;
			wrapDelete(stream.readUInt8(length), volume);
			if (length == 0u) {
				break;
			}

			uint8_t matIdx;
			wrapDelete(stream.readUInt8(matIdx), volume);
			if (matIdx == EMPTY_PALETTE) {
				idx += length;
				continue;
			}
			if (matIdx >= materialAmount) {
				// at least try to load the rest
				idx += length;
				continue;
			}

			const uint8_t index = _paletteMapping[matIdx];
			const Voxel voxel = createVoxel(voxel::VoxelType::Generic, index);

			// left to right, bottom to top, front to back
			for (int i = idx; i < idx + length; i++) {
				const int x = i / (int)(size.y * size.z);
				const int y = (i / (int)size.z) % (int)size.y;
				const int z = i % (int)size.z;
				volume->setVoxel(x, y, z, voxel);
			}
			idx += length;
		}
		SceneGraphNode node(voxel::SceneGraphNodeType::Model);
		node.setVolume(volume, true);
		node.setName(layerName);
		node.setVisible(visible);
		node.setProperty("version", core::string::toString(version));
		node.setProperty("filename", filename);
		node.setTransform(0, transform, true);
		sceneGraph.emplace(core::move(node));
	}

	if (version >= 10) {
		uint8_t surface;
		wrap(stream.readUInt8(surface))
		if (surface) {
			uint32_t startx, starty, startz;
			uint32_t endx, endy, endz;
			uint32_t normal;
			wrap(stream.readUInt32(startx))
			wrap(stream.readUInt32(starty))
			wrap(stream.readUInt32(startz))
			wrap(stream.readUInt32(endx))
			wrap(stream.readUInt32(endy))
			wrap(stream.readUInt32(endz))
			wrap(stream.readUInt32(normal))
		}
		// here might be another byte - but it isn't written everytime
	}

	return true;
}

#undef wrap
#undef wrapBool
#undef wrapDelete

}
