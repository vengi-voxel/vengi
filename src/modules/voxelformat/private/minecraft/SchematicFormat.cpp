/**
 * @file
 */

#include "SchematicFormat.h"
#include "MinecraftPaletteMap.h"
#include "NamedBinaryTag.h"
#include "SchematicIntReader.h"
#include "app/Async.h"
#include "color/Color.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/Array.h"
#include "core/concurrent/Atomic.h"
#include "image/Image.h"
#include "image/ImageType.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include "io/MemoryReadStream.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

#include <glm/common.hpp>
#include <limits>

namespace voxelformat {

static glm::ivec3 parsePosList(const priv::NamedBinaryTag &compound, const core::String &key) {
	const priv::NamedBinaryTag &pos = compound.get(key);
	int x = -1;
	int y = -1;
	int z = -1;
	if (pos.type() == priv::TagType::LIST) {
		const priv::NBTList &positions = *pos.list();
		if (positions.size() != 3) {
			Log::error("Unexpected nbt %s list entry count: %i", key.c_str(), (int)positions.size());
			return glm::ivec3(-1);
		}
		x = positions[0].int32(-1);
		y = positions[1].int32(-1);
		z = positions[2].int32(-1);
	} else if (pos.type() == priv::TagType::COMPOUND) {
		x = pos.get("x").int32(-1);
		y = pos.get("y").int32(-1);
		z = pos.get("z").int32(-1);
	}
	return glm::ivec3(x, y, z);
}

bool SchematicFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
										scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
										const LoadContext &loadctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	palette.minecraft();

	const core::String &extension = core::string::extractExtension(filename);
	if (extension == "bp") {
		// Axiom format is not a zip file, it has a custom binary format
		return loadAxiomBinary(*stream, sceneGraph, palette);
	}

	io::ZipReadStream zipStream(*stream);
	priv::NamedBinaryTagContext ctx;
	ctx.stream = &zipStream;
	const priv::NamedBinaryTag &schematic = priv::NamedBinaryTag::parse(ctx);
	if (!schematic.valid()) {
		Log::error("Could not find 'Schematic' tag");
		return false;
	}

	if (extension == "nbt") {
		const int dataVersion = schematic.get("DataVersion").int32(-1);
		if (loadNbt(schematic, sceneGraph, palette, dataVersion)) {
			return true;
		}
	} else if (extension == "litematic") {
		return loadLitematic(schematic, sceneGraph, palette);
	}

	const int version = schematic.get("Version").int32(-1);
	Log::debug("Load schematic version %i", version);
	switch (version) {
	case 1:
	case 2:
		Log::debug("WorldEdit legacy");
		if (loadSponge1And2(schematic, sceneGraph, palette)) {
			return true;
		}
		// fall through
	case 3:
	default:
		Log::debug("Sponge 3");
		if (loadSponge3(schematic, sceneGraph, palette, version)) {
			return true;
		}
	}
	schematic.print();
	return false;
}

bool SchematicFormat::loadSponge1And2(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
									  palette::Palette &palette) {
	const priv::NamedBinaryTag &blockData = schematic.get("BlockData");
	if (blockData.valid() && blockData.type() == priv::TagType::BYTE_ARRAY) {
		return parseBlockData(schematic, sceneGraph, palette, blockData);
	}
	Log::error("Could not find valid 'BlockData' tags");
	return false;
}

bool SchematicFormat::loadSponge3(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
								  palette::Palette &palette, int version) {
	const priv::NamedBinaryTag &blocks = schematic.get("Blocks");
	if (blocks.valid() && blocks.type() == priv::TagType::BYTE_ARRAY) {
		return parseBlocks(schematic, sceneGraph, palette, blocks, version);
	}
	Log::error("Could not find valid 'Blocks' tags");
	return false;
}

bool SchematicFormat::readLitematicBlockStates(const glm::ivec3 &size, int bits,
											   const priv::NamedBinaryTag &blockStates,
											   scenegraph::SceneGraphNode &node, const SchematicPalette &mcpal) {
	const core::Buffer<int64_t> *data = blockStates.longArray();
	if (data == nullptr) {
		Log::error("Invalid BlockStates - expected long array");
		return false;
	}

	voxel::RawVolume *v = node.volume();
	const palette::Palette &palette = node.palette();
	core::AtomicBool success {true};
	app::for_parallel(0, size.y, [bits, size, data, &mcpal, v, &palette, &success] (int start, int end) {
		voxel::RawVolume::Sampler sampler(v);
		sampler.setPosition(0, start, 0);
		const uint64_t mask = (1 << bits) - 1;
		for (int y = start; y < end; ++y) {
			voxel::RawVolume::Sampler sampler2 = sampler;
			for (int z = 0; z < size.z; ++z) {
				voxel::RawVolume::Sampler sampler3 = sampler2;
				const uint64_t indexyz = size.x * size.z * y + size.x * z;
				for (int x = 0; x < size.x; ++x) {
					const uint64_t index = indexyz + x;
					const uint64_t startBit = index * bits;
					const uint64_t startIdx = startBit / 64;
					const uint64_t rshiftVal = startBit & 63;
					const uint64_t endIdx = startBit % 64 + bits;
					uint64_t id = 0;
					if (endIdx <= 64 && startIdx < data->size()) {
						id = (uint64_t)((*data)[startIdx]) >> rshiftVal & mask;
					} else {
						if (startIdx >= data->size() || startIdx + 1 >= data->size()) {
							Log::error("Invalid BlockStates, out of bounds, start_state: %i, max size: %i, endnum: %i",
									(int)startIdx, (int)data->size(), (int)endIdx);
							success = false;
							return;
						}
						uint64_t move_num_2 = 64 - rshiftVal;
						id =
							(((uint64_t)(*data)[startIdx]) >> rshiftVal | ((uint64_t)(*data)[startIdx + 1]) << move_num_2) & mask;
					}
					if (id == 0) {
						sampler3.movePositiveX();
						continue;
					}
					const int colorIdx = mcpal[id];
					sampler3.setVoxel(voxel::createVoxel(palette, colorIdx));
					sampler3.movePositiveX();
				}
				sampler2.movePositiveY();
			}
			sampler.movePositiveZ();
		}
	});

	return success;
}

image::ImagePtr SchematicFormat::loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
												const LoadContext &ctx) {
	const core::String &extension = core::string::extractExtension(filename);
	if (extension == "bp") {
		core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
		if (!stream) {
			Log::error("Could not load file %s", filename.c_str());
			return {};
		}
		palette::Palette palette;
		palette.minecraft();
		scenegraph::SceneGraph sceneGraph;

		uint32_t magic;
		if (stream->readUInt32(magic) != 0) {
			Log::error("Failed to read Axiom magic number");
			return {};
		}
		const uint32_t AXIOM_MAGIC = FourCC(0x0A, 0xE5, 0xBB, 0x36);
		if (magic != AXIOM_MAGIC) {
			Log::error("Invalid Axiom magic number: 0x%08X", magic);
			return {};
		}

		// skip header
		uint32_t headerTagSize;
		if (stream->readUInt32BE(headerTagSize) != 0) {
			Log::error("Failed to read header tag size");
			return {};
		}
		if (stream->skip(headerTagSize) < 0) {
			Log::error("Failed to skip header");
			return {};
		}

		// Read thumbnail png
		uint32_t thumbnailLength;
		if (stream->readUInt32BE(thumbnailLength) != 0) {
			Log::error("Failed to read thumbnail length");
			return {};
		}
		image::ImagePtr thumbnail = image::createEmptyImage("thumbnail");
		thumbnail->load(image::ImageType::PNG, *stream, thumbnailLength);
		return thumbnail;
	}
	return {};
}

bool SchematicFormat::loadAxiomBinary(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
									   palette::Palette &palette) {
	uint32_t magic;
	if (stream.readUInt32(magic) != 0) {
		Log::error("Failed to read Axiom magic number");
		return false;
	}
	const uint32_t AXIOM_MAGIC = FourCC(0x0A, 0xE5, 0xBB, 0x36);
	if (magic != AXIOM_MAGIC) {
		Log::error("Invalid Axiom magic number: 0x%08X", magic);
		return false;
	}

	// Read header
	// the header is a compound nbt tag
	// * ThumbnailYaw float
	// * ThumbnailPitch float
	// * ContainsAir byte
	// * LockedThumbnail byte
	// * Version long
	// * Author string
	// * Name string
	// * Tags list
	// * BlockCount int
	uint32_t headerTagSize;
	if (stream.readUInt32BE(headerTagSize) != 0) {
		Log::error("Failed to read header tag size");
		return false;
	}
	if (stream.skip(headerTagSize) < 0) {
		Log::error("Failed to skip header");
		return false;
	}

	// Read and skip thumbnail
	uint32_t thumbnailLength;
	if (stream.readUInt32BE(thumbnailLength) != 0) {
		Log::error("Failed to read thumbnail length");
		return false;
	}
	if (stream.skip(thumbnailLength) < 0) {
		Log::error("Failed to skip thumbnail");
		return false;
	}

	// Read block data (gzip compressed NBT)
	uint32_t blockDataLength;
	if (stream.readUInt32BE(blockDataLength) != 0) {
		Log::error("Failed to read block data length");
		return false;
	}
	core::Buffer<uint8_t> blockDataBuffer(blockDataLength);
	const int bytesRead = stream.read(blockDataBuffer.data(), blockDataLength);
	if (bytesRead != (int)blockDataLength) {
		Log::error("Failed to read block data: expected %u bytes, got %d", blockDataLength, bytesRead);
		return false;
	}

	// Decompress the block data
	io::MemoryReadStream memStream(blockDataBuffer.data(), blockDataLength);
	io::ZipReadStream zipStream(memStream);
	priv::NamedBinaryTagContext ctx;
	ctx.stream = &zipStream;
	const priv::NamedBinaryTag &schematic = priv::NamedBinaryTag::parse(ctx);
	if (!schematic.valid()) {
		Log::error("Failed to parse Axiom block data NBT");
		return false;
	}

	return loadAxiom(schematic, sceneGraph, palette);
}

bool SchematicFormat::loadAxiom(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
									palette::Palette &palette) {
	const priv::NamedBinaryTag &blockRegionNbt = schematic.get("BlockRegion");
	if (!blockRegionNbt.valid() || blockRegionNbt.type() != priv::TagType::LIST) {
		Log::error("Could not find valid 'BlockRegion' list tag");
		return false;
	}

	const priv::NBTList &blockRegions = *blockRegionNbt.list();
	if (blockRegions.empty()) {
		Log::error("No block regions found");
		return false;
	}

	// Calculate bounds
	glm::ivec3 mins((std::numeric_limits<int32_t>::max)() / 2);
	glm::ivec3 maxs((std::numeric_limits<int32_t>::min)() / 2);
	for (const auto &regionTag : blockRegions) {
		if (regionTag.type() != priv::TagType::COMPOUND) {
			Log::error("Invalid region tag type");
			return false;
		}
		const glm::ivec3 regionPos(regionTag.get("X").int32(0), regionTag.get("Y").int32(0), regionTag.get("Z").int32(0));
		mins = glm::min(mins, regionPos);
		maxs = glm::max(maxs, regionPos);
	}
	constexpr const int chunkSize = 16;

	// Calculate size in blocks (each region is 16x16x16)
	const glm::ivec3 regionSize = (maxs - mins + glm::ivec3(1)) * chunkSize;
	const voxel::Region region({0, 0, 0}, regionSize - 1);
	if (!region.isValid()) {
		Log::error("Invalid region size: %i %i %i", regionSize.x, regionSize.y, regionSize.z);
		return false;
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setPalette(palette);
	node.setName("Axiom Schematic");
	node.setVolume(new voxel::RawVolume(region), true);

	voxel::RawVolume *volume = node.volume();
	voxel::RawVolume::Sampler sampler(volume);

	// Process each region
	for (const auto &regionTag : blockRegions) {
		const int regionX = regionTag.get("X").int32(0) - mins.x;
		const int regionY = regionTag.get("Y").int32(0) - mins.y;
		const int regionZ = regionTag.get("Z").int32(0) - mins.z;

		// Get block states palette
		const priv::NamedBinaryTag &blockStatesTag = regionTag.get("BlockStates");
		if (!blockStatesTag.valid() || blockStatesTag.type() != priv::TagType::COMPOUND) {
			Log::error("Could not find 'BlockStates' compound");
			return false;
		}

		const priv::NamedBinaryTag &paletteTag = blockStatesTag.get("palette");
		if (!paletteTag.valid() || paletteTag.type() != priv::TagType::LIST) {
			Log::error("Could not find 'palette' list");
			return false;
		}

		const priv::NBTList &paletteNbt = *paletteTag.list();
		SchematicPalette mcpal;
		mcpal.resize(paletteNbt.size());
		int paletteSize = 0;
		for (const auto &palNbt : paletteNbt) {
			const priv::NamedBinaryTag &materialName = palNbt.get("Name");
			if (!materialName.valid()) {
				Log::warn("Missing Name in palette entry");
				mcpal[paletteSize++] = 0;
				continue;
			}
			mcpal[paletteSize++] = findPaletteIndex(materialName.string()->c_str(), 1);
		}

		// Get block data
		const priv::NamedBinaryTag &dataTag = blockStatesTag.get("data");
		if (!dataTag.valid() || dataTag.type() != priv::TagType::LONG_ARRAY) {
			continue;
		}
		const core::Buffer<int64_t> *data = dataTag.longArray();
		core::Array<uint32_t, chunkSize * chunkSize * chunkSize> blockStateData;
		// Calculate bits per value
		const int n = (int)paletteNbt.size();
		int bitsPerValue = 4;
		while ((1 << bitsPerValue) < n) {
			++bitsPerValue;
		}

		int index = 0;
		const uint64_t mask = (1ULL << bitsPerValue) - 1;
		for (int64_t num : *data) {
			for (int i = 0; i < (64 / bitsPerValue) && index < (int)blockStateData.size(); ++i) {
				blockStateData[index++] = (uint32_t)(num & mask);
				num >>= bitsPerValue;
			}
		}
		int i = 0;
		const int chunkPosX = regionX * chunkSize;
		const int chunkPosY = regionY * chunkSize;
		const int chunkPosZ = regionZ * chunkSize;
		for (int z = 0; z < chunkSize; ++z) {
			for (int y = 0; y < chunkSize; ++y) {
				sampler.setPosition(chunkPosX, chunkPosY + y, chunkPosZ + z);
				for (int x = 0; x < chunkSize; ++x) {
					uint8_t colorIdx = mcpal[blockStateData[i++]];
					sampler.setVoxel(voxel::createVoxel(node.palette(), colorIdx));
					sampler.movePositiveX();
				}
			}
		}
	}

	if (sceneGraph.emplace(core::move(node)) == InvalidNodeId) {
		Log::error("Failed to add node to the scenegraph");
		return false;
	}

	return true;
}

bool SchematicFormat::loadLitematic(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
									palette::Palette &palette) {
	const priv::NamedBinaryTag &versionNbt = schematic.get("Version");
	if (versionNbt.valid() && versionNbt.type() == priv::TagType::INT) {
		const int version = versionNbt.int32();
		Log::debug("version: %i", version);
		const priv::NamedBinaryTag &regions = schematic.get("Regions");
		if (!regions.compound()) {
			Log::error("Could not find valid 'Regions' compound tag");
			return false;
		}
		const priv::NBTCompound regionsCompound = *regions.compound();
		for (const auto &regionEntry : regionsCompound) {
			const priv::NamedBinaryTag &regionCompound = regionEntry->second;
			const core::String &name = regionEntry->first;
			const glm::ivec3 &pos = parsePosList(regionCompound, "Position");
			const glm::ivec3 &size = glm::abs(parsePosList(regionCompound, "Size"));
			const voxel::Region region({0, 0, 0}, size - 1);
			if (!region.isValid()) {
				Log::error("Invalid region mins: %i %i %i maxs: %i %i %i", pos.x, pos.y, pos.z, size.x, size.y, size.z);
				return false;
			}
			const priv::NamedBinaryTag &blockStatesPalette = regionCompound.get("BlockStatePalette");
			if (!blockStatesPalette.valid() || blockStatesPalette.type() != priv::TagType::LIST) {
				Log::error("Could not find 'BlockStatePalette'");
				return false;
			}

			const priv::NBTList &blockStatePaletteNbt = *blockStatesPalette.list();
			SchematicPalette mcpal;
			mcpal.resize(blockStatePaletteNbt.size());
			int paletteSize = 0;
			for (const auto &palNbt : blockStatePaletteNbt) {
				const priv::NamedBinaryTag &materialName = palNbt.get("Name");
				mcpal[paletteSize++] = findPaletteIndex(materialName.string()->c_str(), 1);
			}
			const int n = (int)blockStatePaletteNbt.size();
			int bits = 0;
			while (n > (1 << bits)) {
				++bits;
			}
			bits = core_max(bits, 2);

			const priv::NamedBinaryTag &blockStates = regionCompound.get("BlockStates");
			if (!blockStates.valid() || blockStates.type() != priv::TagType::LONG_ARRAY) {
				Log::error("Could not find 'BlockStates'");
				return false;
			}
			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
			node.setPalette(palette);
			node.setName(name);
			node.setVolume(new voxel::RawVolume(region), true);
			if (!readLitematicBlockStates(size, bits, blockStates, node, mcpal)) {
				Log::error("Failed to read 'BlockStates'");
				return false;
			}
			if (sceneGraph.emplace(core::move(node)) == InvalidNodeId) {
				Log::error("Failed to add node to the scenegraph");
				return false;
			}
		}
		return true;
	}
	Log::error("Could not find valid 'Version' tag");
	return false;
}

bool SchematicFormat::loadNbt(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
							  palette::Palette &palette, int dataVersion) {
	const priv::NamedBinaryTag &blocks = schematic.get("blocks");
	if (blocks.valid() && blocks.type() == priv::TagType::LIST) {
		const priv::NBTList &list = *blocks.list();
		glm::ivec3 mins((std::numeric_limits<int32_t>::max)() / 2);
		glm::ivec3 maxs((std::numeric_limits<int32_t>::min)() / 2);
		for (const priv::NamedBinaryTag &compound : list) {
			if (compound.type() != priv::TagType::COMPOUND) {
				Log::error("Unexpected nbt type: %i", (int)compound.type());
				return false;
			}
			const glm::ivec3 v = parsePosList(compound, "pos");
			mins = (glm::min)(mins, v);
			maxs = (glm::max)(maxs, v);
		}
		const voxel::Region region(mins, maxs);
		voxel::RawVolume *volume = new voxel::RawVolume(region);
		for (const priv::NamedBinaryTag &compound : list) {
			const int state = compound.get("state").int32();
			const glm::ivec3 v = parsePosList(compound, "pos");
			volume->setVoxel(v, voxel::createVoxel(palette, state));
		}
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		const priv::NamedBinaryTag &author = schematic.get("author");
		if (author.valid() && author.type() == priv::TagType::STRING) {
			node.setProperty("Author", author.string());
		}
		node.setVolume(volume, true);
		node.setPalette(palette);
		int nodeId = sceneGraph.emplace(core::move(node));
		return nodeId != InvalidNodeId;
	}
	Log::error("Could not find valid 'blocks' tags");
	return false;
}

bool SchematicFormat::parseBlockData(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
									 palette::Palette &palette, const priv::NamedBinaryTag &blockData) {
	const core::Buffer<int8_t> *blocks = blockData.byteArray();
	if (blocks == nullptr) {
		Log::error("Invalid BlockData - expected byte array");
		return false;
	}
	SchematicPalette mcpal;
	const int paletteEntry = parsePalette(schematic, mcpal);

	const int16_t width = schematic.get("Width").int16();
	const int16_t height = schematic.get("Height").int16();
	const int16_t depth = schematic.get("Length").int16();

	if (width == 0 || depth == 0) {
		Log::error("Invalid width or length found");
		return false;
	}

	SchematicIntReader reader(blocks);

	const voxel::Region region(0, 0, 0, width - 1, height - 1, depth - 1);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	// TODO: PERF: FOR_PARALLEL: maybe load all pal indices first?
	voxel::RawVolume::Sampler sampler(volume);
	sampler.setPosition(0, 0, 0);
	for (int y = 0; y < height; y++) {
		voxel::RawVolume::Sampler sampler2 = sampler;
		for (int z = 0; z < depth; z++) {
			voxel::RawVolume::Sampler sampler3 = sampler2;
			for (int x = 0; x < width; x++) {
				int32_t palIdx = 0;
				if (reader.readInt32(palIdx) == -1) {
					break;
				}
				if (palIdx != 0) {
					uint8_t currentPalIdx = (paletteEntry == 0) ? palIdx : mcpal[palIdx];
					if (currentPalIdx != 0) {
						sampler3.setVoxel(voxel::createVoxel(palette, currentPalIdx));
					}
				}
				sampler3.movePositiveX();
			}
			sampler2.movePositiveZ();
			if (reader.eos()) {
				break;
			}
		}
		sampler.movePositiveY();
	}

	const int32_t x = schematic.get("x").int32();
	const int32_t y = schematic.get("y").int32();
	const int32_t z = schematic.get("z").int32();
	volume->translate(glm::ivec3(x, y, z));

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	node.setPalette(palette);
	const int nodeId = sceneGraph.emplace(core::move(node));
	if (nodeId == InvalidNodeId) {
		return false;
	}
	parseMetadata(schematic, sceneGraph, sceneGraph.node(nodeId));
	return true;
}

bool SchematicFormat::parseBlocks(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
								  palette::Palette &palette, const priv::NamedBinaryTag &blocks, int version) {
	SchematicPalette mcpal;
	const int paletteEntry = parsePalette(schematic, mcpal);

	const int16_t width = schematic.get("Width").int16();
	const int16_t height = schematic.get("Height").int16();
	const int16_t depth = schematic.get("Length").int16();

	// TODO: VOXELFORMAT: Support for WorldEdit's AddBlocks is missing
	// * https://github.com/EngineHub/WorldEdit/blob/master/worldedit-core/src/main/java/com/sk89q/worldedit/extent/clipboard/io/MCEditSchematicReader.java#L171
	// * https://github.com/mcedit/mcedit2/blob/master/src/mceditlib/schematic.py#L143
	// * https://github.com/Lunatrius/Schematica/blob/master/src/main/java/com/github/lunatrius/schematica/world/schematic/SchematicAlpha.java

	const voxel::Region region(0, 0, 0, width - 1, height - 1, depth - 1);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	auto fn = [volume, depth, height, width, blocks, paletteEntry, &mcpal, &palette] (int start, int end) {
		voxel::RawVolume::Sampler sampler(volume);
		sampler.setPosition(0, 0, start);
		for (int z = start; z < end; ++z) {
			voxel::RawVolume::Sampler sampler2 = sampler;
			for (int y = 0; y < height; ++y) {
				voxel::RawVolume::Sampler sampler3 = sampler2;
				const int stride = (y * depth + z) * width;
				for (int x = 0; x < width; ++x) {
					const int idx = stride + x;
					const uint8_t palIdx = (*blocks.byteArray())[idx];
					if (palIdx != 0u) {
						uint8_t currentPalIdx;
						if (paletteEntry == 0 || palIdx > paletteEntry) {
							currentPalIdx = palIdx;
						} else {
							currentPalIdx = mcpal[palIdx];
						}
						sampler3.setVoxel(voxel::createVoxel(palette, currentPalIdx));
					}
					sampler3.movePositiveX();
				}
				sampler2.movePositiveY();
			}
			sampler.movePositiveZ();
		}
	};
	app::for_parallel(0, depth, fn);

	const int32_t x = schematic.get("x").int32();
	const int32_t y = schematic.get("y").int32();
	const int32_t z = schematic.get("z").int32();
	volume->translate(glm::ivec3(x, y, z));

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	node.setPalette(palette);
	const int nodeId = sceneGraph.emplace(core::move(node));
	if (nodeId == InvalidNodeId) {
		return false;
	}
	parseMetadata(schematic, sceneGraph, sceneGraph.node(nodeId));
	return true;
}

int SchematicFormat::loadMCEdit2Palette(const priv::NamedBinaryTag &schematic, SchematicPalette &mcpal) const {
	const priv::NamedBinaryTag &blockIds = schematic.get("BlockIDs");
	if (!blockIds.valid()) {
		return -1;
	}
	Log::debug("Found MCEdit2 BlockIDs");
	mcpal.resize(palette::PaletteMaxColors);
	int paletteEntry = 0;
	const int blockCnt = (int)blockIds.compound()->size();
	Log::debug("Loading BlockIDs with %i entries", blockCnt);
	// TODO: FOR_PARALLEL
	for (int i = 0; i < blockCnt; ++i) {
		const priv::NamedBinaryTag &nbt = blockIds.get(core::String::format("%i", i));
		const core::String *value = nbt.string();
		if (value == nullptr) {
			Log::warn("Empty string in BlockIDs for %i", i);
			continue;
		}
		// map to stone on default
		mcpal[i] = findPaletteIndex(*value, 1);
		++paletteEntry;
	}
	return paletteEntry;
}

int SchematicFormat::loadWorldEditPalette(const priv::NamedBinaryTag &schematic, SchematicPalette &mcpal) const {
	const int paletteMax = schematic.get("PaletteMax").int32(-1);
	if (paletteMax == -1) {
		return -1;
	}
	Log::debug("Found WorldEdit PaletteMax %i", paletteMax);
	const priv::NamedBinaryTag &palette = schematic.get("Palette");
	if (palette.valid() && palette.type() == priv::TagType::COMPOUND) {
		if ((int)palette.compound()->size() != paletteMax) {
			return -1;
		}
		mcpal.resize(paletteMax);
		int paletteEntry = 0;
		for (const auto &c : *palette.compound()) {
			const core::String &key = c->key;
			const int palIdx = c->second.int32(-1);
			if (palIdx < 0) {
				Log::warn("Failed to get int value for %s", key.c_str());
				continue;
			}
			if (palIdx >= paletteMax) {
				Log::warn("Palette index %i is out of bounds", palIdx);
				continue;
			}
			// map to stone on default
			mcpal[palIdx] = findPaletteIndex(key, 1);
			++paletteEntry;
		}
		return paletteEntry;
	}
	return -1;
}

// https://github.com/Lunatrius/Schematica/
int SchematicFormat::loadSchematicaPalette(const priv::NamedBinaryTag &schematic, SchematicPalette &mcpal) const {
	const priv::NamedBinaryTag &schematicaMapping = schematic.get("SchematicaMapping");
	if (!schematicaMapping.valid()) {
		return -1;
	}
	if (schematicaMapping.type() != priv::TagType::COMPOUND) {
		return -1;
	}
	Log::debug("Found SchematicaMapping");
	int paletteEntry = 0;
	for (const auto &c : *schematicaMapping.compound()) {
		core::String key = c->key;
		const int palIdx = c->second.int16(-1);
		if (palIdx < 0) {
			Log::warn("Failed to get int value for %s", key.c_str());
			continue;
		}
		// map to stone on default
		mcpal[palIdx] = findPaletteIndex(key, 1);
		++paletteEntry;
	}
	return paletteEntry;
}

int SchematicFormat::parsePalette(const priv::NamedBinaryTag &schematic, SchematicPalette &mcpal) const {
	int paletteEntry = loadMCEdit2Palette(schematic, mcpal);
	if (paletteEntry != -1) {
		return paletteEntry;
	}
	paletteEntry = loadWorldEditPalette(schematic, mcpal);
	if (paletteEntry != -1) {
		return paletteEntry;
	}
	paletteEntry = loadSchematicaPalette(schematic, mcpal);
	if (paletteEntry != -1) {
		return paletteEntry;
	}
	Log::warn("Could not find valid 'BlockIDs' or 'Palette' tag");
	return -1;
}

void SchematicFormat::parseMetadata(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
									scenegraph::SceneGraphNode &node) {
	const priv::NamedBinaryTag &metadata = schematic.get("Metadata");
	if (metadata.valid()) {
		if (const core::String *str = metadata.get("Name").string()) {
			node.setName(*str);
		}
		if (const core::String *str = metadata.get("Author").string()) {
			node.setProperty(scenegraph::PropAuthor, *str);
		}
	}
	const int version = schematic.get("Version").int32(-1);
	if (version != -1) {
		node.setProperty(scenegraph::PropVersion, core::string::toString(version));
	}
	core_assert_msg(node.id() != -1, "The node should already be part of the scene graph");
	for (const auto &e : *schematic.compound()) {
		addMetadata_r(e->key, e->value, sceneGraph, node);
	}
}

void SchematicFormat::addMetadata_r(const core::String &key, const priv::NamedBinaryTag &nbt,
									scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node) {
	switch (nbt.type()) {
	case priv::TagType::COMPOUND: {
		scenegraph::SceneGraphNode compoundNode(scenegraph::SceneGraphNodeType::Group);
		compoundNode.setName(key);
		int nodeId = sceneGraph.emplace(core::move(compoundNode), node.id());
		for (const auto &e : *nbt.compound()) {
			addMetadata_r(e->key, e->value, sceneGraph, sceneGraph.node(nodeId));
		}
		break;
	}
	case priv::TagType::END:
	case priv::TagType::BYTE:
		node.setProperty(key, core::string::toString(nbt.int8()));
		break;
	case priv::TagType::SHORT:
		node.setProperty(key, core::string::toString(nbt.int16()));
		break;
	case priv::TagType::INT:
		node.setProperty(key, core::string::toString(nbt.int32()));
		break;
	case priv::TagType::LONG:
		node.setProperty(key, core::string::toString(nbt.int64()));
		break;
	case priv::TagType::FLOAT:
		node.setProperty(key, core::string::toString(nbt.float32()));
		break;
	case priv::TagType::DOUBLE:
		node.setProperty(key, core::string::toString(nbt.float64()));
		break;
	case priv::TagType::STRING:
		node.setProperty(key, nbt.string());
		break;
	case priv::TagType::LIST: {
		const priv::NBTList &list = *nbt.list();
		scenegraph::SceneGraphNode listNode(scenegraph::SceneGraphNodeType::Group);
		listNode.setName(core::String::format("%s: %i", key.c_str(), (int)list.size()));
		int nodeId = sceneGraph.emplace(core::move(listNode), node.id());
		for (const priv::NamedBinaryTag &e : list) {
			addMetadata_r(key, e, sceneGraph, sceneGraph.node(nodeId));
		}
		break;
	}
	case priv::TagType::BYTE_ARRAY:
		node.setProperty(key, "Byte Array");
		break;
	case priv::TagType::INT_ARRAY:
		node.setProperty(key, "Int Array");
		break;
	case priv::TagType::LONG_ARRAY:
		node.setProperty(key, "Long Array");
		break;
	case priv::TagType::MAX:
		break;
	}
}

bool SchematicFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
								 const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	// save as sponge-3
	const scenegraph::SceneGraph::MergeResult &merged = sceneGraph.merge();
	if (!merged.hasVolume()) {
		Log::error("Failed to merge volumes");
		return false;
	}
	core::ScopedPtr<voxel::RawVolume> mergedVolume(merged.volume());
	const voxel::Region &region = mergedVolume->region();
	const glm::ivec3 &size = region.getDimensionsInVoxels();
	const glm::ivec3 &mins = region.getLowerCorner();

	io::ZipWriteStream zipStream(*stream);

	priv::NBTCompound compound;
	compound.put("Width", (int16_t)size.x);
	compound.put("Height", (int16_t)size.y);
	compound.put("Length", (int16_t)size.z);
	compound.put("x", (int32_t)mins.x);
	compound.put("y", (int32_t)mins.y);
	compound.put("z", (int32_t)mins.z);
	compound.put("Materials", priv::NamedBinaryTag("Alpha"));
	compound.put("Version", 3);

	palette::Palette minecraftPalette;
	minecraftPalette.minecraft();

	const core::VarPtr &schematicType = core::Var::getSafe(cfg::VoxformatSchematicType);
	core::StringMap<int8_t> paletteMap(getPaletteArray().size());
	int paletteIndex = 1;
	{
		core::Buffer<int8_t> blocks;
		blocks.resize((size_t)size.x * (size_t)size.y * (size_t)size.z);

		// TODO: PERF: FOR_PARALLEL
		palette::PaletteLookup palLookup(minecraftPalette);
		voxel::RawVolume::Sampler sampler(mergedVolume);
		sampler.setPosition(mins);
		for (int z = 0; z < size.z; ++z) {
			voxel::RawVolume::Sampler sampler2 = sampler;
			for (int y = 0; y < size.y; ++y) {
				voxel::RawVolume::Sampler sampler3 = sampler2;
				const int stride = (y * size.z + z) * size.x;
				for (int x = 0; x < size.x; ++x) {
					const int idx = stride + x;
					const voxel::Voxel &voxel = sampler3.voxel();
					if (voxel::isAir(voxel.getMaterial())) {
						blocks[idx] = 0;
						sampler3.movePositiveX();
						continue;
					}
					color::RGBA c = merged.palette.color(voxel.getColor());
					const int currentPalIdx = palLookup.findClosestIndex(c);
					const core::String &blockState = findPaletteName(currentPalIdx);
					int8_t blockDataIdx;
					if (blockState.empty()) {
						Log::warn("Failed to find block state for palette index %i", currentPalIdx);
						blockDataIdx = 0;
						if (!paletteMap.empty()) {
							// pick a random one
							blockDataIdx = paletteMap.begin()->second;
						}
					} else {
						auto it = paletteMap.find(blockState);
						if (it == paletteMap.end()) {
							blockDataIdx = paletteIndex++;
							Log::debug("New block state: %s -> %i", blockState.c_str(), blockDataIdx);
							paletteMap.put(blockState, blockDataIdx);
						} else {
							blockDataIdx = it->second;
						}
					}

					Log::debug("Set block state %s at %i %i %i to %i", blockState.c_str(), x, y, z,
								(int)blockDataIdx);
					// Store the palette index in block data
					blocks[idx] = blockDataIdx;
					sampler3.movePositiveX();
				}
				sampler2.movePositiveY();
			}
			sampler.movePositiveZ();
		}
		compound.put("Blocks", priv::NamedBinaryTag(core::move(blocks)));
		if (schematicType->strVal() == "mcedit2") {
			priv::NBTCompound paletteTag;
			for (const auto &e : paletteMap) {
				const core::String key = core::string::toString((int)e->second);
				paletteTag.put(key, priv::NamedBinaryTag(e->first));
			}
			compound.put("BlockIDs", core::move(paletteTag));
		} else if (schematicType->strVal() == "worldedit") {
			priv::NBTCompound paletteTag;
			for (const auto &e : paletteMap) {
				paletteTag.put(e->first, (int32_t)e->second);
			}
			compound.put("Palette", core::move(paletteTag));
			compound.put("PaletteMax", (int32_t)paletteMap.size());
		} else if (schematicType->strVal() == "schematica") {
			priv::NBTCompound paletteTag;
			for (const auto &e : paletteMap) {
				paletteTag.put(e->first, (int16_t)e->second);
			}
			compound.put("SchematicaMapping", core::move(paletteTag));
		} else {
			Log::error("Unknown schematic type: %s", schematicType->strVal().c_str());
		}
	}
	const priv::NamedBinaryTag tag(core::move(compound));
	return priv::NamedBinaryTag::write(tag, "Schematic", zipStream);
}

} // namespace voxelformat
