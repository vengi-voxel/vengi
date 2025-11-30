/**
 * @file
 */

#include "SMTPLFormat.h"
#include "SMPalette.h"
#include "color/Color.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load smtpl file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",          \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) != true) {                                                                                              \
		Log::error("Could not load smtpl file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",          \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

bool SMTPLFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
									scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
									const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	uint8_t version;
	wrap(stream->readUInt8(version))
	glm::ivec3 mins;
	wrap(stream->readInt32BE(mins.x))
	wrap(stream->readInt32BE(mins.y))
	wrap(stream->readInt32BE(mins.z))

	glm::ivec3 maxs;
	wrap(stream->readInt32BE(maxs.x))
	wrap(stream->readInt32BE(maxs.y))
	wrap(stream->readInt32BE(maxs.z))

	Log::debug("Region: %i:%i:%i - %i:%i:%i", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);

	uint32_t numBlocks;
	wrap(stream->readUInt32BE(numBlocks))

	Log::debug("Number of blocks: %i", numBlocks);

	if (maxs.x > 2048 || maxs.y > 2048 || maxs.z > 2048) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", maxs.x, maxs.y, maxs.z);
		return false;
	}

	const voxel::Region region(mins, maxs - 1);
	if (!region.isValid()) {
		Log::error("Invalid region: %i:%i:%i - %i:%i:%i", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
		return false;
	}

	loadPalette(palette);

	core::Map<int, int> blockPal;
	for (int i = 0; i < lengthof(BLOCKCOLOR); ++i) {
		blockPal.put(BLOCKCOLOR[i].blockId, palette.getClosestMatch(BLOCKCOLOR[i].color));
	}
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	for (uint32_t i = 0; i < numBlocks; ++i) {
		uint32_t x, y, z;
		wrap(stream->readUInt32BE(x))
		wrap(stream->readUInt32BE(y))
		wrap(stream->readUInt32BE(z))
		// TODO: VOXELFORMAT: the following bytes are handled differently since version > 3
		uint8_t type;
		wrap(stream->readUInt8(type))
#if 0
		uint8_t orientation;
		wrap(stream->readUInt8(orientation))
		uint8_t active;
		wrap(stream->readUInt8(active))
#else
		uint16_t block;
		wrap(stream->readUInt16BE(block))
#endif
		int color = 0;
		blockPal.get(block, color);
		volume->setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, color));
	}
#if 0
	uint32_t connections;
	wrap(stream->readUInt32BE(connections))
	Log::debug("Connections: %i", connections);
	for (uint32_t i = 0; i < connections; ++i) {
		int64_t key;
		wrap(stream->readInt64BE(key))
		int32_t size;
		wrap(stream->readInt32BE(size))
		for (int32_t j = 0; j < size; ++j) {
			int64_t value;
			wrap(stream->readInt64BE(value))
		}
	}

	if (version >= 2) {
		uint32_t texts;
		wrap(stream->readUInt32BE(texts))
		Log::debug("Texts: %i", texts);
		for (uint32_t i = 0; i < texts; ++i) {
			int64_t key;
			wrap(stream->readInt64BE(key))
			core::String text;
			wrap(stream->readPascalStringUInt16BE(text))
		}
		if (version >= 3) {
			uint32_t filters;
			wrap(stream->readUInt32BE(filters))
			Log::debug("Filters: %i", filters);
			for (uint32_t i = 0; i < filters; ++i) {
				int64_t key;
				wrap(stream->readInt64BE(key))
				int32_t size;
				wrap(stream->readInt32BE(size))
				for (int32_t j = 0; j < size; ++j) {
					int16_t key;
					wrap(stream->readInt16BE(key))
					int32_t value;
					wrap(stream->readInt32BE(value))
				}
			}
			uint32_t productions;
			wrap(stream->readUInt32BE(productions))
			Log::debug("Productions: %i", productions);
			for (uint32_t i = 0; i < productions; ++i) {
				int64_t key;
				wrap(stream->readInt64BE(key))
				int16_t text;
				wrap(stream->readInt16BE(text))
			}
			if (version >= 5) {
				int32_t productionLimits;
				wrap(stream->readInt32BE(productionLimits))
				Log::debug("Production limits: %i", productionLimits);
				for (int32_t i = 0; i < productionLimits; ++i) {
					int64_t key;
					wrap(stream->readInt64BE(key))
					int32_t text;
					wrap(stream->readInt32BE(text))
				}
				int32_t fillUpFilters;
				wrap(stream->readInt32BE(fillUpFilters))
				Log::debug("Fill up filters: %i", fillUpFilters);
				for (int32_t i = 0; i < fillUpFilters; ++i) {
					int64_t key;
					wrap(stream->readInt64BE(key))
					int32_t size;
					wrap(stream->readInt32BE(size))
					for (int32_t j = 0; j < size; ++j) {
						int16_t key;
						wrap(stream->readInt16BE(key))
						int32_t value;
						wrap(stream->readInt32BE(value))
					}
				}
			}
		}
	}
#endif
	node.setName(core::string::extractFilename(filename));
	node.setPalette(palette);
	sceneGraph.emplace(core::move(node));
	return true;
}

#undef wrap
#undef wrapBool

#define wrapBool(read)                                                                                                 \
	if ((read) != true) {                                                                                              \
		Log::error("Could not write smtpl file: Failed to write to stream " CORE_STRINGIFY(read) " (line %i)",         \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

static uint16_t resolveBlockId(const palette::Palette &starMadePal, const palette::Palette &palette, const uint8_t colorIdx) {
	int match = starMadePal.getClosestMatch(palette.color(colorIdx));
	if (match == palette::PaletteColorNotFound) {
		match = colorIdx;
	}
	return BLOCKS_FOR_INTERNAL_PAL[match];
}

bool SMTPLFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							 const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	core_assert(node);

	const voxel::Region &region = node->region();
	voxel::RawVolume::Sampler sampler(node->volume());
	const glm::ivec3 &lower = region.getLowerCorner();

	const uint32_t width = region.getWidthInVoxels();
	const uint32_t height = region.getHeightInVoxels();
	const uint32_t depth = region.getDepthInVoxels();

	uint8_t version = 3;
	wrapBool(stream->writeUInt8(version))
	glm::ivec3 mins{0};
	wrapBool(stream->writeInt32BE(mins.x))
	wrapBool(stream->writeInt32BE(mins.y))
	wrapBool(stream->writeInt32BE(mins.z))

	wrapBool(stream->writeUInt32BE(width))
	wrapBool(stream->writeUInt32BE(height))
	wrapBool(stream->writeUInt32BE(depth))

	palette::Palette starMadePal;
	loadPalette(starMadePal);

	const int32_t numBlocks = voxelutil::countVoxels(*node->volume());
	wrapBool(stream->writeUInt32BE(numBlocks))
	Log::debug("Number of blocks: %i", numBlocks);

	int32_t blocks = 0;
	for (uint32_t x = 0u; x < width; ++x) {
		for (uint32_t y = 0u; y < height; ++y) {
			for (uint32_t z = 0u; z < depth; ++z) {
				core_assert_always(sampler.setPosition(lower.x + x, lower.y + y, lower.z + z));
				const voxel::Voxel &voxel = sampler.voxel();
				if (voxel::isAir(voxel.getMaterial())) {
					continue;
				}
				wrapBool(stream->writeUInt32BE(x))
				wrapBool(stream->writeUInt32BE(y))
				wrapBool(stream->writeUInt32BE(z))
				wrapBool(stream->writeUInt8(0)) // type
				uint16_t blockId = resolveBlockId(starMadePal, node->palette(), voxel.getColor());
				wrapBool(stream->writeUInt16BE(blockId))
				++blocks;
			}
		}
	}
	if (blocks != numBlocks) {
		Log::error("Number of blocks written does not match the expected number: %i != %i", blocks, numBlocks);
		return false;
	}
	wrapBool(stream->writeUInt32BE(0)) // no connections
	wrapBool(stream->writeUInt32BE(0)) // no texts
	wrapBool(stream->writeUInt32BE(0)) // no inventory filters
	wrapBool(stream->writeUInt32BE(0)) // no productions

	return true;
}

void SMTPLFormat::loadPalette(palette::Palette &palette) {
	for (int i = 0; i < lengthof(BLOCKCOLOR); ++i) {
		uint8_t index = 0;
		const color::RGBA rgba = BLOCKCOLOR[i].color;
		palette.tryAdd(rgba, true, &index);
		for (int j = 0; j < lengthof(BLOCKEMITCOLOR); ++j) {
			if (BLOCKEMITCOLOR[j].blockId != BLOCKCOLOR[i].blockId) {
				continue;
			}
			const color::RGBA emit = BLOCKEMITCOLOR[j].color;
			const float factor = color::getDistance(emit, rgba, color::Distance::HSB);
			palette.setEmit(index, 1.0f - factor);
		}
	}
}

size_t SMTPLFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
								const LoadContext &ctx) {
	loadPalette(palette);
	return palette.size();
}

#undef wrapBool

} // namespace voxelformat
