/**
 * @file
 */

#include "MCRFormat.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/StringMap.h"
#include "io/File.h"
#include "io/MemoryReadStream.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include "private/NamedBinaryTag.h"
#include "private/MinecraftPaletteMap.h"
#include "voxel/MaterialColor.h"
#include "voxel/Palette.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxelformat/SceneGraph.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeMerger.h"

#include <glm/common.hpp>

namespace voxelformat {

#define wrap(expression)                                                                                               \
	do {                                                                                                               \
		if ((expression) != 0) {                                                                                       \
			Log::error("Could not load file: Not enough data in stream " CORE_STRINGIFY(#expression) " at " SDL_FILE   \
																									 ":%i",            \
					   SDL_LINE);                                                                                      \
			return false;                                                                                              \
		}                                                                                                              \
	} while (0)

bool MCRFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette) {
	const int64_t length = stream.size();
	if (length < SECTOR_BYTES) {
		Log::error("File does not contain enough data");
		return false;
	}

	core::String name = core::string::extractFilenameWithExtension(filename.toLower());
	int chunkX = 0;
	int chunkZ = 0;
	char type = 'a';
	if (SDL_sscanf(name.c_str(), "r.%i.%i.mc%c", &chunkX, &chunkZ, &type) != 3) {
		Log::warn("Failed to parse the region chunk boundaries from filename %s (%i.%i.%c)", name.c_str(), chunkX, chunkZ, type);
	}

	palette.minecraft();
	switch (type) {
	case 'r':	// Region file format
	case 'a': { // Anvil file format
		const int64_t fileSize = stream.remaining();
		if (fileSize < 2l * SECTOR_BYTES) {
			Log::error("This region file has not enough data for the 8kb header");
			return false;
		}

		for (int i = 0; i < SECTOR_INTS; ++i) {
			uint8_t raw[3];
			wrap(stream.readUInt8(raw[0]));
			wrap(stream.readUInt8(raw[1]));
			wrap(stream.readUInt8(raw[2]));
			wrap(stream.readUInt8(_offsets[i].sectorCount));

			_offsets[i].offset = ((raw[0] << 16) + (raw[1] << 8) + raw[2]) * SECTOR_BYTES;
		}

		for (int i = 0; i < SECTOR_INTS; ++i) {
			uint32_t lastModValue;
			wrap(stream.readUInt32BE(lastModValue));
		}

		// might be an empty region file
		if (stream.eos()) {
			Log::warn("Empty region file");
			return false;
		}

		const bool success = loadMinecraftRegion(sceneGraph, stream, palette);
		return success;
	}
	}
	Log::error("Unkown file type given: %c", type);
	return false;
}

bool MCRFormat::loadMinecraftRegion(SceneGraph &sceneGraph, io::SeekableReadStream &stream, const voxel::Palette &palette) {
	for (int i = 0; i < SECTOR_INTS; ++i) {
		if (_offsets[i].sectorCount == 0u || _offsets[i].offset < sizeof(_offsets)) {
			continue;
		}
		if (_offsets[i].offset + 6 >= (uint32_t)stream.size()) {
			return false;
		}
		if (stream.seek(_offsets[i].offset) == -1) {
			continue;
		}
		if (!readCompressedNBT(sceneGraph, stream, i, palette)) {
			Log::error("Failed to load minecraft chunk section %i for offset %u", i, (int)_offsets[i].offset);
			return false;
		}
	}

	return true;
}

bool MCRFormat::readCompressedNBT(SceneGraph &sceneGraph, io::SeekableReadStream &stream, int sector, const voxel::Palette &palette) {
	uint32_t nbtSize;
	wrap(stream.readUInt32BE(nbtSize));
	if (nbtSize == 0) {
		Log::debug("Empty nbt chunk found");
		return true;
	}

	if (nbtSize > 0x1FFFFFF) {
		Log::error("Size of nbt data exceeds the max allowed value: %u", nbtSize);
		return false;
	}

	uint8_t version;
	wrap(stream.readUInt8(version));
	if (version != VERSION_GZIP && version != VERSION_DEFLATE) {
		Log::error("Unsupported version found: %u", version);
		return false;
	}

	// the version is included in the length
	--nbtSize;

	io::ZipReadStream zipStream(stream, (int)nbtSize);
	priv::NamedBinaryTagContext ctx;
	ctx.stream = &zipStream;
	const priv::NamedBinaryTag &root = priv::NamedBinaryTag::parse(ctx);
	if (!root.valid()) {
		Log::error("Could not parse nbt structure");
		return false;
	}

	voxel::RawVolume *volume;
	// https://minecraft.fandom.com/wiki/Data_version
	const int32_t dataVersion = root.get("DataVersion").int32();
	Log::debug("Found data version %i", dataVersion);
	if (dataVersion >= 2844) {
		volume = parseSections(dataVersion, root, sector, palette);
		if (volume == nullptr) {
			return false;
		}
	} else {
		volume = parseLevelCompound(dataVersion, root, sector, palette);
		if (volume == nullptr) {
			return false;
		}
	}
	SceneGraphNode node(SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	node.setPalette(palette);
	sceneGraph.emplace(core::move(node));
	return true;
}

int MCRFormat::getVoxel(int dataVersion, const priv::NamedBinaryTag &data, const glm::ivec3 &pos) {
	const uint32_t i = pos.y * MAX_SIZE * MAX_SIZE + pos.z * MAX_SIZE + pos.x;
	if (i >= data.byteArray()->size()) {
		Log::error("Byte array index out of bounds: %u/%i", i, (int)data.byteArray()->size());
		return -1;
	}
	const int val = (int)(uint8_t)(*data.byteArray())[i];
	if (val < 0) {
		Log::error("Invalid value: %i", val);
		return -1;
	}
	return val;
}

voxel::RawVolume *MCRFormat::error(SectionVolumes &volumes) {
	for (voxel::RawVolume *v : volumes) {
		delete v;
	}
	return nullptr;
}

voxel::RawVolume* MCRFormat::finalize(SectionVolumes& volumes, int xPos, int zPos) {
	if (volumes.empty()) {
		Log::error("No volumes found at %i:%i", xPos, zPos);
		return nullptr;
	}
	// TODO: only merge connected y chunks - don't fill empty chunks - just a waste of memory
	voxel::RawVolume *merged = voxelutil::merge(volumes);
	for (voxel::RawVolume* v : volumes) {
		delete v;
	}
	merged->translate(glm::ivec3(xPos * MAX_SIZE, 0, zPos * MAX_SIZE));
	voxel::RawVolume *cropped = voxelutil::cropVolume(merged);
	delete merged;
	return cropped;
}

bool MCRFormat::parseBlockStates(int dataVersion, const voxel::Palette &palette, const priv::NamedBinaryTag &data, SectionVolumes &volumes, int sectionY, const MinecraftSectionPalette &secPal) {
	Log::debug("Parse block states");
	const bool hasData = data.type() == priv::TagType::LONG_ARRAY && !data.longArray()->empty();

	constexpr glm::ivec3 mins(0, 0, 0);
	constexpr glm::ivec3 maxs(MAX_SIZE - 1, MAX_SIZE - 1, MAX_SIZE - 1);
	const voxel::Region region(mins, maxs);
	voxel::RawVolume *v = new voxel::RawVolume(region);
	voxel::RawVolumeWrapper wrapper(v);

	if (secPal.pal.empty()) {
		if (data.type() != priv::TagType::BYTE_ARRAY) {
			Log::error("Unknown block data type: %i for version %i", (int)data.type(), dataVersion);
			delete v;
			return false;
		}
		glm::ivec3 sPos;
		for (sPos.y = 0; sPos.y < MAX_SIZE; ++sPos.y) {
			for (sPos.z = 0; sPos.z < MAX_SIZE; ++sPos.z) {
				for (sPos.x = 0; sPos.x < MAX_SIZE; ++sPos.x) {
					const int color = getVoxel(dataVersion, data, sPos);
					if (color < 0) {
						Log::error("Failed to load voxel at position %i:%i:%i (dataversion: %i)", sPos.x, sPos.y, sPos.z, dataVersion);
						delete v;
						return false;
					}
					if (color) {
						const uint8_t palColIdx = palette.getClosestMatch(secPal.mcpal.colors[color]);
						const voxel::Voxel voxel = voxel::createVoxel(palette, palColIdx);
						wrapper.setVoxel(sPos, voxel);
					}
				}
			}
		}
	} else if (hasData) {
		if (data.type() != priv::TagType::LONG_ARRAY) {
			Log::error("Unknown block data type: %i for version %i", (int)data.type(), dataVersion);
			delete v;
			return false;
		}

		const core::DynamicArray<int64_t> &blockStates = *data.longArray();

		uint8_t blocks[4096];
		int bsCnt = 0;
		size_t bitCnt = 0;
		if (dataVersion < 2529) {
			const size_t bitSize = (data.longArray()->size()) * 64 / 4096;
			const uint32_t bitMask = (1 << bitSize) - 1;
			for (int i = 0; i < 4096; i++) {
				if (bitCnt + bitSize <= 64) {
					const uint64_t blockState = blockStates[bsCnt];
					const uint64_t blockIndex = (blockState >> bitCnt) & bitMask;
					if (blockIndex < secPal.pal.size()) {
						blocks[i] = secPal.pal[blockIndex];
					} else {
						blocks[i] = 0;
					}
					bitCnt += bitSize;
					if (bitCnt == 64) {
						bitCnt = 0;
						bsCnt++;
					}
				} else {
					const uint64_t blockState1 = blockStates[bsCnt++];
					const uint64_t blockState2 = blockStates[bsCnt];
					uint32_t blockIndex = (blockState1 >> bitCnt) & bitMask;
					bitCnt += bitSize;
					bitCnt -= 64;
					blockIndex += (blockState2 << (bitSize - bitCnt)) & bitMask;
					if (blockIndex < secPal.pal.size()) {
						blocks[i] = secPal.pal[blockIndex];
					} else {
						blocks[i] = 0;
					}
				}
			}
		} else {
			const size_t bitSize = secPal.numBits;
			const uint32_t bitMask = (1 << bitSize) - 1;
			for (int i = 0; i < 4096; i++) {
				const uint64_t blockState = blockStates[bsCnt];
				const uint64_t blockIndex = (blockState >> bitCnt) & bitMask;
				if (blockIndex < secPal.pal.size()) {
					blocks[i] = secPal.pal[blockIndex];
				} else {
					blocks[i] = 0;
				}
				bitCnt += bitSize;
				if (bitCnt + bitSize > 64) {
					bsCnt++;
					bitCnt = 0;
				}
			}
		}

		glm::ivec3 sPos;
		for (sPos.y = 0; sPos.y < MAX_SIZE; ++sPos.y) {
			for (sPos.z = 0; sPos.z < MAX_SIZE; ++sPos.z) {
				for (sPos.x = 0; sPos.x < MAX_SIZE; ++sPos.x) {
					const uint16_t i = sPos.y * MAX_SIZE * MAX_SIZE + sPos.z * MAX_SIZE + sPos.x;
					const uint8_t color = blocks[i];
					if (color) {
						const uint8_t palColIdx = palette.getClosestMatch(secPal.mcpal.colors[color]);
						const voxel::Voxel voxel = voxel::createVoxel(palette, palColIdx);
						wrapper.setVoxel(sPos, voxel);
					}
				}
			}
		}
	}

	if (wrapper.dirtyRegion().isValid()) {
		glm::ivec3 translate;
		translate.x = 0;
		translate.y = sectionY * MAX_SIZE;
		translate.z = 0;

		v->translate(translate);
		volumes.push_back(v);
	} else {
		delete v;
	}
	return true;
}

voxel::RawVolume *MCRFormat::parseSections(int dataVersion, const priv::NamedBinaryTag &root, int sector, const voxel::Palette &pal) {
	const priv::NamedBinaryTag &sections = root.get("sections");
	if (!sections.valid()) {
		Log::error("Could not find 'sections' tag");
		return nullptr;
	}
	if (sections.type() != priv::TagType::LIST) {
		Log::error("Unexpected tag type found for 'sections' tag: %i", (int)sections.type());
		return nullptr;
	}

	const int32_t xPos = root.get("xPos").int32();
	const int32_t zPos = root.get("zPos").int32();

	Log::debug("xpos: %i, zpos: %i", xPos, zPos);

	const priv::NBTList &sectionsList = *sections.list();
	Log::debug("Found %i sections", (int)sectionsList.size());
	if (sectionsList.empty()) {
		Log::warn("Empty region - no sections found - version: %i", dataVersion);
		return nullptr;
	}
	SectionVolumes volumes;
	for (const priv::NamedBinaryTag &section : sectionsList) {
		const priv::NamedBinaryTag &blockStates = section.get("block_states");
		if (!blockStates.valid()) {
			Log::error("Could not find 'block_states'");
			return error(volumes);
		}
		const priv::NamedBinaryTag &ylvl = section.get("Y");
		if (!ylvl.valid()) {
			Log::debug("Could not find Y int in section compound");
			continue;
		}
		const int8_t sectionY = ylvl.int8();
		if (sectionY == -1) {
			Log::debug("Skip empty section compound");
			continue;
		}
		Log::debug("Y level for section compound: %i", (int)sectionY);

		const priv::NamedBinaryTag &palette = blockStates.get("palette");
		if (!palette.valid()) {
			Log::error("Could not find 'palette'");
			return error(volumes);
		}
		MinecraftSectionPalette secPal;
		secPal.mcpal.minecraft();
		if (!parsePaletteList(dataVersion, palette, secPal)) {
			Log::error("Could not parse palette chunk");
			return error(volumes);
		}
		const priv::NamedBinaryTag &data = blockStates.get("data");
		if (!parseBlockStates(dataVersion, pal, data, volumes, sectionY, secPal)) {
			Log::error("Failed to parse 'data' tag");
			return error(volumes);
		}
	}
	return finalize(volumes, xPos, zPos);
}

voxel::RawVolume *MCRFormat::parseLevelCompound(int dataVersion, const priv::NamedBinaryTag &root, int sector, const voxel::Palette &pal) {
	const priv::NamedBinaryTag &levels = root.get("Level");
	if (!levels.valid()) {
		Log::error("Could not find 'Level' tag");
		return nullptr;
	}
	if (levels.type() != priv::TagType::COMPOUND) {
		Log::error("Invalid type for 'Level' tag: %i", (int)levels.type());
		return nullptr;
	}
	const int32_t xPos = levels.get("xPos").int32();
	const int32_t zPos = levels.get("zPos").int32();

	if (dataVersion >= 1976) {
		const core::String *tagStatus = root.get("Status").string();
		if (tagStatus == nullptr) {
			Log::debug("Status for level node wasn't found (version: %i)", dataVersion);
		} else if (*tagStatus != "full") {
			Log::debug("Status for level node is not full but %s (version: %i)", tagStatus->c_str(), dataVersion);
		}
	} else if (dataVersion >= 1628) {
		const core::String *tagStatus = levels.get("Status").string();
		if (tagStatus == nullptr) {
			Log::debug("Status for level node wasn't found (version: %i)", dataVersion);
		} else if (*tagStatus != "postprocessed") {
			Log::debug("Status for level node is not postprocessed but %s (version: %i)", tagStatus->c_str(), dataVersion);
		}
	}

	const priv::NamedBinaryTag &sections = levels.get("Sections");
	if (!sections.valid()) {
		Log::error("Could not find 'Sections' tag");
		return nullptr;
	}
	if (sections.type() != priv::TagType::LIST) {
		Log::error("Invalid type for 'Sections' tag: %i", (int)sections.type());
		return nullptr;
	}
	const priv::NBTList &sectionsList = *sections.list();
	Log::debug("Found %i sections", (int)sectionsList.size());
	if (sectionsList.empty()) {
		Log::warn("Empty region - no sections found - version: %i", dataVersion);
		root.print();
		return nullptr;
	}
	SectionVolumes volumes;
	for (const priv::NamedBinaryTag &section : sectionsList) {
		const priv::NamedBinaryTag &ylvl = section.get("Y");
		if (!ylvl.valid()) {
			Log::debug("Could not find Y int in section compound");
			continue;
		}
		const int8_t sectionY = ylvl.int8();
		if (sectionY == -1) {
			Log::debug("Skip empty section compound");
			continue;
		}
		Log::debug("Y level for section compound: %i", (int)sectionY);

		MinecraftSectionPalette secPal;
		secPal.mcpal.minecraft();

		const priv::NamedBinaryTag &palette = section.get("Palette");
		if (palette.valid()) {
			if (!parsePaletteList(dataVersion, palette, secPal)) {
				Log::error("Failed to parse 'Palette' tag");
				return error(volumes);
			}
		} else {
			Log::debug("Could not find a Palette compound in section %i", dataVersion);
		}

		// TODO:"Data"(byte_array)
		//const priv::NamedBinaryTag &data = section.get("Data");
		const core::String &tagId = dataVersion <= 1343 ? "Blocks" : "BlockStates";
		const priv::NamedBinaryTag &blockStates = section.get(tagId);
		if (!blockStates.valid()) {
			Log::debug("Could not find '%s'", tagId.c_str());
			continue;
		}
		if (!parseBlockStates(dataVersion, pal, blockStates, volumes, sectionY, secPal)) {
			Log::error("Failed to parse '%s' tag", tagId.c_str());
			return error(volumes);
		}
	}
	return finalize(volumes, xPos, zPos);
}

bool MCRFormat::parsePaletteList(int dataVersion, const priv::NamedBinaryTag &palette, MinecraftSectionPalette &sectionPal) {
	if (palette.type() != priv::TagType::LIST) {
		Log::error("Invalid type for palette: %i", (int)palette.type());
		return false;
	}
	const priv::NBTList &paletteList = *palette.list();
	const size_t paletteCount = paletteList.size();
	if (paletteCount > 512u) {
		Log::error("Palette overflow");
		return false;
	}
	sectionPal.pal.resize(paletteCount);
	sectionPal.numBits = (uint32_t)glm::max(glm::ceil(glm::log2((float)paletteCount)), 4.0f);

	int paletteEntry = 0;
	for (const priv::NamedBinaryTag &block : paletteList) {
		if (block.type() != priv::TagType::COMPOUND) {
			Log::error("Invalid block type %i", (int)block.type());
			return false;
		}

		for (const auto &entry : *block.compound()) {
			if (entry->key != "Name") {
				continue;
			}
			const priv::NamedBinaryTag &nbt = entry->value;
			const core::String *value = nbt.string();
			if (value == nullptr) {
				continue;
			}
			sectionPal.pal[paletteEntry] = findPaletteIndex(*value);
		}
		++paletteEntry;
	}
	return true;
}

#undef wrap

#define wrapBool(write) \
	if ((write) == false) { \
		Log::error("Could not save mcr file: " CORE_STRINGIFY(write)); \
		return false; \
	}

bool MCRFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, ThumbnailCreator thumbnailCreator) {
	for (int i = 0; i < SECTOR_INTS; ++i) {
		uint8_t raw[3] = {0, 0, 0}; // TODO
		_offsets[i].sectorCount = 0; // TODO

		core_assert(_offsets[i].offset < sizeof(_offsets));
		wrapBool(stream.writeUInt8(raw[0]));
		wrapBool(stream.writeUInt8(raw[1]));
		wrapBool(stream.writeUInt8(raw[2]));
		wrapBool(stream.writeUInt8(_offsets[i].sectorCount));
	}

	for (int i = 0; i < SECTOR_INTS; ++i) {
		uint32_t lastModValue = 0u;
		wrapBool(stream.writeUInt32BE(lastModValue));
	}

	return saveMinecraftRegion(sceneGraph, stream);
}

bool MCRFormat::saveMinecraftRegion(const voxelformat::SceneGraph &sceneGraph, io::SeekableWriteStream& stream) {
	for (int i = 0; i < SECTOR_INTS; ++i) {
		if (_offsets[i].sectorCount == 0u) {
			continue;
		}
		if (!saveCompressedNBT(sceneGraph, stream, i)) {
			Log::error("Failed to save minecraft chunk section %i for offset %u", i, (int)_offsets[i].offset);
			return false;
		}
	}
	return true;
}

bool MCRFormat::saveCompressedNBT(const voxelformat::SceneGraph &sceneGraph, io::SeekableWriteStream& stream, int sector) {
	const int64_t sizeOffset = stream.pos();
	wrapBool(stream.writeUInt32BE(0));
	// the version is included in the length
	const int64_t nbtStartOffset = stream.pos();
	wrapBool(stream.writeUInt8(VERSION_GZIP));

	io::ZipWriteStream zipStream(stream);
	priv::NBTCompound root;
	root.put("DataVersion", 2844);
	int x = 0; // TODO
	int y = 0; // TODO
	root.put("xPos", x);
	root.put("yPos", y);
	priv::NBTList sections;
	if (!saveSections(sceneGraph, sections, sector)) {
		Log::error("Failed to save section for sector %i", sector);
		return false;
	}
	root.emplace("sections", priv::NamedBinaryTag(core::move(sections)));
	const priv::NamedBinaryTag tag(core::move(root));
	if (!priv::NamedBinaryTag::write(tag, "", zipStream)) {
		Log::error("Failed to write nbt");
		return false;
	}
	const int64_t nbtEndOffset = stream.pos();

	const int64_t nbtSize = nbtEndOffset - nbtStartOffset;
	if (stream.seek(sizeOffset) == -1) {
		Log::error("Failed to seek for nbt size pos");
		return false;
	}
	wrapBool(stream.writeUInt32BE(nbtSize))
	return stream.seek(nbtEndOffset) != -1;
}

bool MCRFormat::saveSections(const voxelformat::SceneGraph &sceneGraph, priv::NBTList &sections, int sector) {
#if 0
	for (const SceneGraphNode &node : sceneGraph) {
		priv::NBTCompound blockStates;
		blockStates.put("data", 0); // parseBlockStates
		blockStates.put("palette", 0); // parsePaletteList

		priv::NBTCompound section;
		section.put("Y", (int8_t)0);
		section.put("block_states", priv::NamedBinaryTag(core::move(blockStates)));

		sections.emplace_back(core::move(section));
	}
	return true;
#else
	return false;
#endif
}

#undef wrapBool

} // namespace voxelformat
