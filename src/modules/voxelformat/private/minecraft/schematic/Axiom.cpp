/**
 * @file
 */

#include "../MinecraftPaletteMap.h"
#include "../NamedBinaryTag.h"
#include "Util.h"
#include "core/FourCC.h"
#include "core/StringUtil.h"
#include "io/MemoryReadStream.h"
#include "io/ZipReadStream.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/RawVolume.h"

namespace voxelformat {
namespace axiom {

static bool loadAxiom(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
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
		const glm::ivec3 regionPos(regionTag.get("X").int32(0), regionTag.get("Y").int32(0),
								   regionTag.get("Z").int32(0));
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

		// minecraft:structure_void is air in Axiom schematics
		const priv::NBTList &paletteNbt = *paletteTag.list();
		schematic::SchematicPalette mcpal;
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
		const int nbtPaletteSize = (int)paletteNbt.size();
		int bits = 0;
		while (nbtPaletteSize > (1 << bits)) {
			++bits;
		}
		bits = core_max(bits, 2);

		int index = 0;
		const uint64_t mask = (1ULL << bits) - 1;
		for (int64_t num : *data) {
			for (int i = 0; i < (64 / bits) && index < (int)blockStateData.size(); ++i) {
				blockStateData[index++] = (uint32_t)(num & mask);
				num >>= bits;
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

bool loadGroupsPalette(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, palette::Palette &palette) {
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
	uint32_t headerTagSize;
	if (stream.readUInt32BE(headerTagSize) != 0) {
		Log::error("Failed to read header tag size");
		return false;
	}
	priv::NamedBinaryTagContext headerCtx;
	io::BufferedReadWriteStream headerStream(stream, headerTagSize);
	headerCtx.stream = &headerStream;
	const priv::NamedBinaryTag &header = priv::NamedBinaryTag::parse(headerCtx);
	if (!header.valid()) {
		Log::error("Failed to parse Axiom header compound NBT");
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
	const priv::NamedBinaryTag &blockData = priv::NamedBinaryTag::parse(ctx);
	if (!blockData.valid()) {
		Log::error("Failed to parse Axiom block data NBT");
		return false;
	}

	const int blockCount = header.get("BlockCount").int32(0);
	const int8_t containsAir = header.get("ContainsAir").int8(0);
	const int8_t lockedThumbnail = header.get("LockedThumbnail").int8(0);
	const float thumbnailYaw = header.get("ThumbnailYaw").float32(0.0f);
	const float thumbnailPitch = header.get("ThumbnailPitch").float32(0.0f);
	// TODO: VOXELFORMAT: header.get("Tags").list()
	Log::debug("Block count: %i", blockCount);
	Log::debug("Contains air: %i", (int)containsAir);
	Log::debug("Locked thumbnail: %i", (int)lockedThumbnail);
	Log::debug("Thumbnail yaw: %.2f", thumbnailYaw);
	Log::debug("Thumbnail pitch: %.2f", thumbnailPitch);

	if (loadAxiom(blockData, sceneGraph, palette)) {
		scenegraph::SceneGraphNode &rootNode = sceneGraph.node(0);
		if (const core::String *author = header.get("Author").string()) {
			rootNode.setProperty(scenegraph::PropAuthor, *author);
		}
		if (const core::String *name = header.get("Name").string()) {
			rootNode.setProperty(scenegraph::PropTitle, *name);
		}
		const int64_t version = header.get("Version").int64(0);
		rootNode.setProperty(scenegraph::PropVersion, core::string::toString(version));
		return true;
	}
	return false;
}

image::ImagePtr loadScreenshot(io::SeekableReadStream *stream) {
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

} // namespace axiom
} // namespace voxelformat
