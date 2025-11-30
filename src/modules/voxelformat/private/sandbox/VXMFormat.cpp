/**
 * @file
 */

#include "VXMFormat.h"
#include "color/Color.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/MaterialColor.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include <glm/common.hpp>

namespace voxelformat {

static const uint8_t EMPTY_PALETTE = 0xFFu;

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load vxm file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

#define wrapDelete(read, mem)                                                                                          \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load vxm file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		delete (mem);                                                                                                  \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) != true) {                                                                                              \
		Log::error("Could not load vxm file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

bool VXMFormat::writeRLE(io::WriteStream &stream, int length, const voxel::Voxel &voxel,
						 const palette::Palette &nodePalette, const palette::Palette &palette) const {
	if (length == 0) {
		return true;
	}
	wrapBool(stream.writeUInt8(length))
	if (voxel::isAir(voxel.getMaterial())) {
		wrapBool(stream.writeUInt8(EMPTY_PALETTE))
	} else {
		const core::RGBA color = nodePalette.color(voxel.getColor());
		const int palIndex = palette.getClosestMatch(color, EMPTY_PALETTE);
		if (palIndex < 0) {
			Log::error("Got palette index %i for %s", palIndex, core::Color::print(color, true).c_str());
		}
		core_assert(palIndex != EMPTY_PALETTE);
		wrapBool(stream.writeUInt8(palIndex))
	}
	return true;
}

image::ImagePtr VXMFormat::loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
										  const LoadContext &ctx) {
	const core::String image = filename + ".png";
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(image));
	if (!stream) {
		Log::error("Could not load file %s", image.c_str());
		return image::ImagePtr();
	}
	return image::loadImage(image, *stream, stream->size());
}

bool VXMFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	wrapBool(stream->writeUInt32(FourCC('V', 'X', 'M', 'C')));
	glm::vec3 pivot(0.5f);

	if (const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode()) {
		pivot = node->worldPivot();
	}

	const voxel::Region &region = sceneGraph.region();
	const glm::ivec3 &mins = region.getLowerCorner();
	const glm::ivec3 &maxs = region.getUpperCorner();
	const uint32_t width = region.getWidthInVoxels();
	const uint32_t height = region.getHeightInVoxels();
	const uint32_t depth = region.getDepthInVoxels();

	// we have to flip depth with height for our own coordinate system
	wrapBool(stream->writeUInt32(width))
	wrapBool(stream->writeUInt32(height))
	wrapBool(stream->writeUInt32(depth))

	wrapBool(stream->writeFloat(pivot.x));
	wrapBool(stream->writeFloat(pivot.y));
	wrapBool(stream->writeFloat(pivot.z));

	wrapBool(stream->writeBool(false)); // surface
	// has surface - set to false otherwise
	// the following data is needed:
	// 3 int start
	// 3 int end
	// 1 int normal possible values: [0,1][2,3][4,5]
	// followed by surface width * surface height bytes

	wrapBool(stream->writeFloat(0.0f)); // lod scale
	wrapBool(stream->writeFloat(0.0f)); // lod pivot x
	wrapBool(stream->writeFloat(0.0f)); // lod pivot y
	wrapBool(stream->writeFloat(0.0f)); // lod pivot z

	int lodLevels = 1;
	wrapBool(stream->writeInt32(lodLevels));
	for (int lod = 0; lod < lodLevels; ++lod) {
		wrapBool(stream->writeUInt32(0)); // texture dim x
		wrapBool(stream->writeUInt32(0)); // texture dim y
		wrapBool(stream->writeUInt32(0)); // zipped size for the rgba texture(s)
		// followed by the compressed data

		for (int i = 0; i < 6; ++i) {
			const int quadAmount = 0;
			wrapBool(stream->writeUInt32(quadAmount));
#if 0
			for (int i = 0; i < quadAmount; ++i) {
				for (int j = 0; j < 4; ++j) {
					stream->writeFloat(vertices[j].x);
					stream->writeFloat(vertices[j].y);
					stream->writeFloat(vertices[j].z);
					stream->writeInt32(u[j]);
					stream->writeInt32(v[j]);
				}
			}
#endif
		}
	}

	palette::Palette palette = sceneGraph.mergePalettes(true, EMPTY_PALETTE);
	int numColors = palette.colorCount();
	if (numColors >= palette::PaletteMaxColors) {
		numColors = palette::PaletteMaxColors - 1;
	}
	if (numColors <= 0) {
		Log::error("No palette entries found - can't save");
		return false;
	}

	// albedo palette
	for (int i = 0; i < numColors; ++i) {
		const core::RGBA &matcolor = palette.color(i);
		wrapBool(stream->writeUInt8(matcolor.r))
		wrapBool(stream->writeUInt8(matcolor.g))
		wrapBool(stream->writeUInt8(matcolor.b))
		wrapBool(stream->writeUInt8(matcolor.a))
	}
	for (int i = numColors; i < palette::PaletteMaxColors; ++i) {
		wrapBool(stream->writeUInt8(255))
		wrapBool(stream->writeUInt8(0))
		wrapBool(stream->writeUInt8(255))
		wrapBool(stream->writeUInt8(255))
	}
	// emissive palette
	for (int i = 0; i < numColors; ++i) {
		const bool emissive = palette.hasEmit(i);
		if (emissive) {
			const core::RGBA &glowcolor = palette.emitColor(i);
			wrapBool(stream->writeUInt8(glowcolor.r))
			wrapBool(stream->writeUInt8(glowcolor.g))
			wrapBool(stream->writeUInt8(glowcolor.b))
			wrapBool(stream->writeUInt8(glowcolor.a))
		} else {
			wrapBool(stream->writeUInt8(0))
			wrapBool(stream->writeUInt8(0))
			wrapBool(stream->writeUInt8(0))
			wrapBool(stream->writeUInt8(255))
		}
	}
	for (int i = numColors; i < palette::PaletteMaxColors; ++i) {
		wrapBool(stream->writeUInt8(255))
		wrapBool(stream->writeUInt8(0))
		wrapBool(stream->writeUInt8(255))
		wrapBool(stream->writeUInt8(255))
	}

	int chunkAmount = 0;
	wrapBool(stream->writeUInt8(chunkAmount));
	// always false - but the format support multiple chunks - so leave this here as a reference
	for (int c = 0; c < chunkAmount; ++c) {
		core::String id = "";
		wrapBool(stream->writeString(id, true))
		uint8_t offset = 0;
		wrapBool(stream->writeUInt8(offset))
		uint8_t chunkLength = 0;
		wrapBool(stream->writeUInt8(chunkLength))
	}

	wrapBool(stream->writeUInt8(numColors))
	for (int i = 0; i < numColors; ++i) {
		const core::RGBA &matcolor = palette.color(i);
		wrapBool(stream->writeUInt8(matcolor.b))
		wrapBool(stream->writeUInt8(matcolor.g))
		wrapBool(stream->writeUInt8(matcolor.r))
		wrapBool(stream->writeUInt8(matcolor.a))
		const bool emissive = palette.hasEmit(i);
		wrapBool(stream->writeBool(emissive))
	}

	int models = (int)sceneGraph.size(scenegraph::SceneGraphNodeType::AllModels);
	if (models > 0xFF) {
		Log::warn("Failed to save to vxm - max model size exceeded");
		return false;
	}
	wrapBool(stream->writeUInt8(models))

	for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		wrapBool(stream->writeString(node.name(), true))
		wrapBool(stream->writeBool(node.visible()))

		uint32_t rleCount = 0u;
		voxel::Voxel prevVoxel;
		bool firstLoop = true;

		voxel::RawVolume::Sampler sampler(sceneGraph.resolveVolume(node));
		sampler.setPosition(maxs.x, mins.y, mins.z);
		for (uint32_t x = 0u; x < width; ++x) {
			voxel::RawVolume::Sampler sampler2 = sampler;
			for (uint32_t y = 0u; y < height; ++y) {
				voxel::RawVolume::Sampler sampler3 = sampler2;
				for (uint32_t z = 0u; z < depth; ++z) {
					// this might fail - vxm uses the same size for each model - we don't
					// in case the position is outside of the node volume, we are putting
					// the border voxel of the volume into the file
					const voxel::Voxel &voxel = sampler3.voxel();
					if (prevVoxel.getColor() != voxel.getColor() || voxel.getMaterial() != prevVoxel.getMaterial() ||
						rleCount >= 255) {
						wrapBool(writeRLE(*stream, rleCount, prevVoxel, node.palette(), palette))
						prevVoxel = voxel;
						rleCount = 0;
					} else if (firstLoop) {
						firstLoop = false;
						prevVoxel = voxel;
					}
					++rleCount;
					sampler3.movePositiveZ();
				}
				sampler2.movePositiveY();
			}
			sampler.moveNegativeX();
		}
		if (rleCount > 0) {
			wrapBool(writeRLE(*stream, rleCount, prevVoxel, node.palette(), palette))
		}

		wrapBool(stream->writeUInt8(0));
	}
	wrapBool(stream->writeBool(false));
	// has surface - set to false otherwise
	// the following data is needed:
	// 3 int start
	// 3 int end
	// 1 int normal possible values: [0,1][2,3][4,5]

	return true;
}

bool VXMFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
								  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	uint8_t magic[4];
	wrap(stream->readUInt8(magic[0]))
	wrap(stream->readUInt8(magic[1]))
	wrap(stream->readUInt8(magic[2]))
	wrap(stream->readUInt8(magic[3]))
	if (magic[0] != 'V' || magic[1] != 'X' || magic[2] != 'M') {
		Log::error("Could not load vxm file: Invalid magic found (%c%c%c%c)", magic[0], magic[1], magic[2], magic[3]);
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

	if (version < 3 || version > 12) {
		Log::error("Could not load vxm file: Unsupported version found (%i)", version);
		return false;
	}

	glm::vec3 normalizedPivot = glm::vec3{0.5f, 0.0f, 0.5f};
	glm::uvec3 size(0);
	Log::debug("Found vxm%i", version);
	if (version >= 6) {
		wrap(stream->readUInt32(size.x));
		wrap(stream->readUInt32(size.y));
		wrap(stream->readUInt32(size.z));
	}
	if (version >= 5) {
		wrap(stream->readFloat(normalizedPivot.x));
		wrap(stream->readFloat(normalizedPivot.y));
		wrap(stream->readFloat(normalizedPivot.z));
	}
	if (version >= 9) {
		uint8_t surface;
		wrap(stream->readUInt8(surface))
		if (surface) {
			uint32_t skipWidth = 0u;
			uint32_t skipHeight = 0u;
			uint32_t startx, starty, startz;
			uint32_t endx, endy, endz;
			uint32_t normal;
			// since version 10 the start and end values are floats
			// but for us this fact doesn't matter
			wrap(stream->readUInt32(startx))
			wrap(stream->readUInt32(starty))
			wrap(stream->readUInt32(startz))
			wrap(stream->readUInt32(endx))
			wrap(stream->readUInt32(endy))
			wrap(stream->readUInt32(endz))
			wrap(stream->readUInt32(normal))
			if (version >= 10) {
				wrap(stream->readUInt32(skipWidth))
				wrap(stream->readUInt32(skipHeight))
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
			stream->skip(skipWidth * skipHeight);
		}
	}
	if (version >= 8) {
		float dummy;				   // since version 'A'
		wrap(stream->readFloat(dummy)); // lod scale
		wrap(stream->readFloat(dummy)); // lod pivot x
		wrap(stream->readFloat(dummy)); // lod pivot y
		wrap(stream->readFloat(dummy)); // lod pivot z
	}

	uint32_t lodLevels = 1;
	if (version >= 7) {
		wrap(stream->readUInt32(lodLevels));
	}
	for (uint32_t lodLevel = 0u; lodLevel < lodLevels; ++lodLevel) {
		glm::uvec2 textureDim;
		wrap(stream->readUInt32(textureDim.x));
		wrap(stream->readUInt32(textureDim.y));
		if (glm::any(glm::greaterThan(textureDim, glm::uvec2(2048)))) {
			Log::warn("Size of texture exceeds the max allowed value");
			return false;
		}

		if (version >= 11) {
			uint32_t pixelSize;
			wrap(stream->readUInt32(pixelSize));
			stream->skip(pixelSize); // zipped pixel data
		} else if (version == 3) {
			uint8_t byte;
			do {
				wrap(stream->readUInt8(byte));
				if (byte != 0u) {
					stream->skip(3);
				}
			} while (byte != 0);
		} else {
			uint32_t texAmount;
			wrap(stream->readUInt32(texAmount));
			if (texAmount > 0xFFFF) {
				Log::warn("Size of textures exceeds the max allowed value: %i", texAmount);
				return false;
			}

			Log::debug("texAmount: %i", (int)texAmount);
			for (uint32_t t = 0u; t < texAmount; t++) {
				char textureId[1024];
				wrapBool(stream->readString(sizeof(textureId), textureId, true))
				if (version >= 6) {
					uint32_t texZipped;
					wrap(stream->readUInt32(texZipped));
					stream->skip(texZipped);
				} else {
					Log::debug("tex: %i: %s", (int)t, textureId);
					uint32_t px = 0u;
					for (;;) {
						uint8_t rleStride;
						wrap(stream->readUInt8(rleStride));
						if (rleStride == 0u) {
							break;
						}

						struct TexColor {
							glm::u8vec3h rgb;
						};
						static_assert(sizeof(TexColor) == 3, "Unexpected TexColor size");
						stream->skip(sizeof(TexColor));
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
			wrap(stream->readUInt32(quadAmount));
			if (quadAmount > 0x40000U) {
				Log::warn("Size of quads exceeds the max allowed value");
				return false;
			}
			struct QuadVertex {
				glm::vec3h pos;
				glm::ivec2h uv;
			};
			static_assert(sizeof(QuadVertex) == 20, "Unexpected QuadVertex size");
			stream->skip(quadAmount * 4 * sizeof(QuadVertex));
		}
	}

	if (version <= 5) {
		wrap(stream->readUInt32(size.x));
		wrap(stream->readUInt32(size.y));
		wrap(stream->readUInt32(size.z));
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
		// TODO: MATERIAL: parse the material emit data
		stream->skip(256l * 4l); // palette data rgba for albedo materials
		stream->skip(256l * 4l); // palette data rgba for emissive materials
		uint8_t chunkAmount;	// palette chunks
		wrap(stream->readUInt8(chunkAmount));
		for (int i = 0; i < (int)chunkAmount; ++i) {
			char chunkId[1024];
			wrapBool(stream->readString(sizeof(chunkId), chunkId, true))
			stream->skip(1); // chunk offset
			stream->skip(1); // chunk length
		}
	}

	uint8_t materialAmount;
	wrap(stream->readUInt8(materialAmount));
	Log::debug("Palette of size %i", (int)materialAmount);

	for (int i = 0; i < (int)materialAmount; ++i) {
		uint8_t blue;
		wrap(stream->readUInt8(blue));
		uint8_t green;
		wrap(stream->readUInt8(green));
		uint8_t red;
		wrap(stream->readUInt8(red));
		uint8_t alpha;
		wrap(stream->readUInt8(alpha));
		if (version > 3) {
			uint8_t emissive;
			wrap(stream->readUInt8(emissive));
			palette.setColor(i, core::RGBA(red, green, blue, alpha));
			if (emissive) {
				palette.setEmit(i, 1.0f);
			}
		}
	}
	palette.setSize(materialAmount);

	const voxel::Region region(glm::ivec3(0), glm::ivec3(size) - 1);

	uint8_t maxModels = 1;
	if (version >= 12) {
		wrap(stream->readUInt8(maxModels));
	}

	for (uint8_t model = 0; model < maxModels; ++model) {
		int idx = 0;
		bool visible = true;
		char modelName[1024];
		if (version >= 12) {
			wrapBool(stream->readString(sizeof(modelName), modelName, true))
			visible = stream->readBool();
		} else {
			core::String::formatBuf(modelName, sizeof(modelName), "Model %i", model);
		}
		voxel::RawVolume *volume = new voxel::RawVolume(region);
		for (;;) {
			uint8_t length;
			wrapDelete(stream->readUInt8(length), volume);
			if (length == 0u) {
				break;
			}

			uint8_t matIdx;
			wrapDelete(stream->readUInt8(matIdx), volume);
			if (matIdx == EMPTY_PALETTE) {
				idx += length;
				continue;
			}
			if (matIdx >= materialAmount) {
				// at least try to load the rest
				idx += length;
				continue;
			}

			const voxel::Voxel voxel = voxel::createVoxel(palette, matIdx);

			// left to right, bottom to top, front to back
			// TODO: PERF: use volume sampler - see BinVoxFormat.cpp
			for (int i = idx; i < idx + length; i++) {
				const int x = i / (int)(size.y * size.z);
				const int y = (i / (int)size.z) % (int)size.y;
				const int z = i % (int)size.z;
				volume->setVoxel(size.x - x - 1, y, z, voxel);
			}
			idx += length;
		}
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(volume, true);
		node.setName(modelName);
		node.setVisible(visible);
		node.setPivot(normalizedPivot);
		node.setPalette(palette);
		node.setProperty("vxmversion", core::string::toString(version));
		node.setProperty("filename", filename);
		sceneGraph.emplace(core::move(node));
	}

	if (version >= 10) {
		uint8_t surface;
		wrap(stream->readUInt8(surface))
		if (surface) {
			uint32_t startx, starty, startz;
			uint32_t endx, endy, endz;
			uint32_t normal;
			wrap(stream->readUInt32(startx))
			wrap(stream->readUInt32(starty))
			wrap(stream->readUInt32(startz))
			wrap(stream->readUInt32(endx))
			wrap(stream->readUInt32(endy))
			wrap(stream->readUInt32(endz))
			wrap(stream->readUInt32(normal))
		}
		// here might be another byte - but it isn't written everytime
		uint8_t templateModelResized;
		stream->peekUInt8(templateModelResized);
		if (!stream->eos() && templateModelResized != 127) {
			stream->readBool(); // templateModelResized
		}
		if (!stream->eos()) {
			uint8_t sentinelByte;
			wrap(stream->readUInt8(sentinelByte))
			if (sentinelByte != 127) {
				Log::warn("Sentinel byte is not 127");
				return true; // true anyway, because the additional palette data is optional
			}
			uint8_t selectedPalette;
			wrap(stream->readUInt8(selectedPalette))
			if (selectedPalette != 255) {
				for (int i = 0; i < 255; ++i) {
					uint32_t color;
					wrap(stream->readUInt32(color))
					/*bool emissive =*/ stream->readBool();
				}
			}
		}
	}

	return true;
}

#undef wrap
#undef wrapBool
#undef wrapDelete

} // namespace voxelformat
