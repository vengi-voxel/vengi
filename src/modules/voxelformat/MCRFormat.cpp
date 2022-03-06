/**
 * @file
 */

#include "MCRFormat.h"
#include "SDL_endian.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "io/File.h"
#include "io/ZipReadStream.h"
#include "io/MemoryReadStream.h"

#include <glm/common.hpp>

namespace voxel {

#define wrap(expression)                                                                                               \
	do {                                                                                                               \
		if ((expression) != 0) {                                                                                       \
			Log::error("Could not load file: Not enough data in stream " CORE_STRINGIFY(#expression) " at " SDL_FILE   \
																									 ":%i",            \
					   SDL_LINE);                                                                                      \
			return false;                                                                                              \
		}                                                                                                              \
	} while (0)

#define wrapBool(expression)                                                                                           \
	do {                                                                                                               \
		if (!(expression)) {                                                                                           \
			Log::error("Could not load file: Not enough data in stream " CORE_STRINGIFY(#expression) " at " SDL_FILE   \
																									 ":%i",            \
					   SDL_LINE);                                                                                      \
			return false;                                                                                              \
		}                                                                                                              \
	} while (0)

static const char *nbtTagNames[] = {
	"END",
	"BYTE",
	"SHORT",
	"INT",
	"LONG",
	"FLOAT",
	"DOUBLE",
	"BYTE_ARRAY",
	"STRING",
	"LIST",
	"COMPOUND",
	"INT_ARRAY",
	"LONG_ARRAY"
};

void MCRFormat::reset() {
	core_memset(_chunkTimestamps, 0, sizeof(_chunkTimestamps));
}

bool MCRFormat::loadGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	reset();
	const int64_t length = stream.size();
	if (length < SECTOR_BYTES) {
		Log::error("File does not contain enough data");
		return false;
	}

	core::String name = filename.toLower();
	int chunkX = 0;
	int chunkZ = 0;
	char type = 'a';
	if (SDL_sscanf(name.c_str(), "r.%i.%i.mc%c", &chunkX, &chunkZ, &type) != 3) {
		Log::warn("Failed to parse the region chunk boundaries from filename %s", name.c_str());
		const core::String &extension = core::string::extractExtension(filename);
		type = extension.last();
		chunkX = 0;
		chunkZ = 0;
	}

	switch (type) {
	case 'r':
	case 'a': {
		const int64_t fileSize = stream.remaining();
		if (fileSize <= 2l * SECTOR_BYTES) {
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
			_chunkTimestamps[i] = lastModValue;
		}

		const bool success = loadMinecraftRegion(sceneGraph, stream, chunkX, chunkZ);
		return success;
	}
	}
	Log::error("Unkown file type given: %c", type);
	return false;
}

bool MCRFormat::loadMinecraftRegion(SceneGraph &sceneGraph, io::SeekableReadStream &stream, int chunkX, int chunkZ) {
	for (int i = 0; i < SECTOR_INTS; ++i) {
		if (_offsets[i].sectorCount == 0u || _offsets[i].offset < sizeof(_offsets)) {
			Log::debug("Skip sector %i (offset: %u)", i, _offsets[i].offset);
			continue;
		}
		if (_offsets[i].offset + 6 >= (uint32_t)stream.size()) {
			Log::error("Exceeded stream boundaries: %u, %i", (int)_offsets[i].offset, (int)stream.size());
			return false;
		}
		if (stream.seek(_offsets[i].offset) == -1) {
			Log::debug("Failed to load minecraft chunk section %i for offset %u", i, (int)_offsets[i].offset);
			continue;
		}
		Log::debug("Chunk %i:%i sector %i", chunkX, chunkZ, i);
		if (!readCompressedNBT(sceneGraph, stream)) {
			Log::error("Failed to load minecraft chunk section %i for offset %u", i, (int)_offsets[i].offset);
			return false;
		}
	}

	return true;
}

bool MCRFormat::readCompressedNBT(SceneGraph &sceneGraph, io::SeekableReadStream &stream) {
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

	const int64_t pos = stream.pos();
	io::ZipReadStream zipStream(stream, nbtSize);
	if (!parseNBTChunk(sceneGraph, zipStream)) {
		return false;
	}
	return stream.seek(pos + nbtSize) != -1;
}

// this list was found in enkiMI by Doug Binks
static const struct {
	const core::String name;
	const uint8_t id;
} TYPES[] = {{"minecraft:air", 0},
			 {"minecraft:stone", 1},
			 {"minecraft:andesite", 1},
			 {"minecraft:seagrass", 2},
			 {"minecraft:grass", 2},
			 {"minecraft:grass_block", 2},
			 {"minecraft:dirt", 3},
			 {"minecraft:cobblestone", 4},
			 {"minecraft:oak_planks", 5},
			 {"minecraft:oak_sapling", 6},
			 {"minecraft:bedrock", 7},
			 {"minecraft:flowing_water", 8},
			 {"minecraft:water", 9},
			 {"minecraft:flowing_lava", 10},
			 {"minecraft:lava", 11},
			 {"minecraft:sand", 12},
			 {"minecraft:gravel", 13},
			 {"minecraft:gold_ore", 14},
			 {"minecraft:iron_ore", 15},
			 {"minecraft:coal_ore", 16},
			 {"minecraft:oak_log", 17},
			 {"minecraft:acacia_leaves", 18},
			 {"minecraft:spruce_leaves", 18},
			 {"minecraft:oak_leaves", 18},
			 {"minecraft:jungle_leaves", 18},
			 {"minecraft:birch_leaves", 18},
			 {"minecraft:sponge", 19},
			 {"minecraft:glass", 20},
			 {"minecraft:lapis_ore", 21},
			 {"minecraft:lapis_block", 22},
			 {"minecraft:dispenser", 23},
			 {"minecraft:sandstone", 24},
			 {"minecraft:note_block", 25},
			 {"minecraft:red_bed", 26},
			 {"minecraft:powered_rail", 27},
			 {"minecraft:detector_rail", 28},
			 {"minecraft:sticky_piston", 29},
			 {"minecraft:cobweb", 30},
			 {"minecraft:tall_grass", 31},
			 {"minecraft:tall_seagrass", 31},
			 {"minecraft:dead_bush", 32},
			 {"minecraft:piston", 33},
			 {"minecraft:piston_head", 34},
			 {"minecraft:white_concrete", 35},
			 {"minecraft:dandelion", 37},
			 {"minecraft:poppy", 38},
			 {"minecraft:brown_mushroom", 39},
			 {"minecraft:red_mushroom", 40},
			 {"minecraft:gold_block", 41},
			 {"minecraft:iron_block", 42},
			 {"minecraft:smooth_stone_slab", 43},
			 {"minecraft:stone_slab", 44},
			 {"minecraft:brick_wall", 45},
			 {"minecraft:bricks", 45},
			 {"minecraft:tnt", 46},
			 {"minecraft:bookshelf", 47},
			 {"minecraft:mossy_cobblestone", 48},
			 {"minecraft:obsidian", 49},
			 {"minecraft:torch", 50},
			 {"minecraft:fire", 51},
			 {"minecraft:spawner", 52},
			 {"minecraft:oak_stairs", 53},
			 {"minecraft:chest", 54},
			 {"minecraft:redstone_wire", 55},
			 {"minecraft:diamond_ore", 56},
			 {"minecraft:diamond_block", 57},
			 {"minecraft:crafting_table", 58},
			 {"minecraft:wheat", 59},
			 {"minecraft:farmland", 60},
			 {"minecraft:furnace", 61},
			 {"minecraft:campfire", 62},
			 {"minecraft:oak_sign", 63},
			 {"minecraft:oak_door", 64},
			 {"minecraft:ladder", 65},
			 {"minecraft:rail", 66},
			 {"minecraft:stone_stairs", 67},
			 {"minecraft:oak_wall_sign", 68},
			 {"minecraft:lever", 69},
			 {"minecraft:stone_pressure_plate", 70},
			 {"minecraft:iron_door", 71},
			 {"minecraft:oak_pressure_plate", 72},
			 {"minecraft:redstone_ore", 73},
			 {"minecraft:red_concrete", 74},
			 {"minecraft:redstone_wall_torch", 75},
			 {"minecraft:redstone_torch", 76},
			 {"minecraft:stone_button", 77},
			 {"minecraft:snow_block", 78},
			 {"minecraft:ice", 79},
			 {"minecraft:snow", 80},
			 {"minecraft:cactus", 81},
			 {"minecraft:clay", 82},
			 {"minecraft:bamboo", 83},
			 {"minecraft:jukebox", 84},
			 {"minecraft:oak_fence", 85},
			 {"minecraft:pumpkin", 86},
			 {"minecraft:netherrack", 87},
			 {"minecraft:soul_sand", 88},
			 {"minecraft:glowstone", 89},
			 {"minecraft:portal", 90},
			 {"minecraft:carved_pumpkin", 91},
			 {"minecraft:cake", 92},
			 {"minecraft:repeater", 93},
			 {"minecraft:skeleton_skull", 94},
			 {"minecraft:white_stained_glass", 95},
			 {"minecraft:oak_trapdoor", 96},
			 {"minecraft:turtle_egg", 97},
			 {"minecraft:stone_bricks", 98},
			 {"minecraft:brown_mushroom_block", 99},
			 {"minecraft:red_mushroom_block", 100},
			 {"minecraft:iron_bars", 101},
			 {"minecraft:light_blue_stained_glass_pane", 102},
			 {"minecraft:melon", 103},
			 {"minecraft:pumpkin_stem", 104},
			 {"minecraft:melon_stem", 105},
			 {"minecraft:vine", 106},
			 {"minecraft:oak_fence_gate", 107},
			 {"minecraft:brick_stairs", 108},
			 {"minecraft:stone_brick_stairs", 109},
			 {"minecraft:mycelium", 110},
			 {"minecraft:light_gray_concrete", 111},
			 {"minecraft:nether_brick", 112},
			 {"minecraft:nether_brick_fence", 113},
			 {"minecraft:nether_brick_stairs", 114},
			 {"minecraft:nether_wart", 115},
			 {"minecraft:enchanting_table", 116},
			 {"minecraft:brewing_stand", 117},
			 {"minecraft:cauldron", 118},
			 {"minecraft:end_portal", 119},
			 {"minecraft:end_portal_frame", 120},
			 {"minecraft:end_stone", 121},
			 {"minecraft:dragon_egg", 122},
			 {"minecraft:redstone_lamp", 123},
			 {"minecraft:shroomlight", 124},
			 {"minecraft:oak_wood", 125},
			 {"minecraft:oak_slab", 126},
			 {"minecraft:cocoa", 127},
			 {"minecraft:sandstone_stairs", 128},
			 {"minecraft:emerald_ore", 129},
			 {"minecraft:ender_chest", 130},
			 {"minecraft:tripwire_hook", 131},
			 {"minecraft:tripwire", 132},
			 {"minecraft:emerald_block", 133},
			 {"minecraft:spruce_stairs", 134},
			 {"minecraft:birch_stairs", 135},
			 {"minecraft:jungle_stairs", 136},
			 {"minecraft:command_block", 137},
			 {"minecraft:beacon", 138},
			 {"minecraft:cobblestone_wall", 139},
			 {"minecraft:flower_pot", 140},
			 {"minecraft:carrots", 141},
			 {"minecraft:potatoes", 142},
			 {"minecraft:oak_button", 143},
			 {"minecraft:skeleton_wall_skull", 144},
			 {"minecraft:anvil", 145},
			 {"minecraft:trapped_chest", 146},
			 {"minecraft:light_weighted_pressure_plate", 147},
			 {"minecraft:heavy_weighted_pressure_plate", 148},
			 {"minecraft:comparator", 149},
			 {"minecraft:chain", 150},
			 {"minecraft:daylight_detector", 151},
			 {"minecraft:redstone_block", 152},
			 {"minecraft:nether_quartz_ore", 153},
			 {"minecraft:hopper", 154},
			 {"minecraft:quartz_block", 155},
			 {"minecraft:quartz_stairs", 156},
			 {"minecraft:activator_rail", 157},
			 {"minecraft:dropper", 158},
			 {"minecraft:pink_stained_glass", 159},
			 {"minecraft:white_stained_glass_pane", 160},
			 {"minecraft:dead_brain_coral", 161},
			 {"minecraft:acacia_planks", 162},
			 {"minecraft:acacia_stairs", 163},
			 {"minecraft:dark_oak_stairs", 164},
			 {"minecraft:slime_block", 165},
			 {"minecraft:barrier", 166},
			 {"minecraft:iron_trapdoor", 167},
			 {"minecraft:prismarine", 168},
			 {"minecraft:sea_lantern", 169},
			 {"minecraft:hay_block", 170},
			 {"minecraft:white_carpet", 171},
			 {"minecraft:coarse_dirt", 172},
			 {"minecraft:coal_block", 173},
			 {"minecraft:packed_ice", 174},
			 {"minecraft:orange_concrete", 175},
			 {"minecraft:white_banner", 176},
			 {"minecraft:white_wall_banner", 177},
			 {"minecraft:white_concrete_powder", 178},
			 {"minecraft:red_sandstone", 179},
			 {"minecraft:red_sandstone_stairs", 180},
			 {"minecraft:red_sandstone_wall", 181},
			 {"minecraft:red_sandstone_slab", 182},
			 {"minecraft:spruce_fence_gate", 183},
			 {"minecraft:birch_fence_gate", 184},
			 {"minecraft:jungle_fence_gate", 185},
			 {"minecraft:dark_oak_fence_gate", 186},
			 {"minecraft:acacia_fence_gate", 187},
			 {"minecraft:spruce_fence", 188},
			 {"minecraft:birch_fence", 189},
			 {"minecraft:jungle_fence", 190},
			 {"minecraft:dark_oak_fence", 191},
			 {"minecraft:acacia_fence", 192},
			 {"minecraft:spruce_door", 193},
			 {"minecraft:birch_door", 194},
			 {"minecraft:jungle_door", 195},
			 {"minecraft:acacia_door", 196},
			 {"minecraft:dark_oak_door", 197},
			 {"minecraft:end_rod", 198},
			 {"minecraft:chorus_plant", 199},
			 {"minecraft:chorus_flower", 200},
			 {"minecraft:purpur_block", 201},
			 {"minecraft:purpur_pillar", 202},
			 {"minecraft:purpur_stairs", 203},
			 {"minecraft:purple_stained_glass", 204},
			 {"minecraft:purpur_slab", 205},
			 {"minecraft:end_stone_bricks", 206},
			 {"minecraft:beetroots", 207},
			 {"minecraft:grass_path", 208},
			 {"minecraft:end_gateway", 209},
			 {"minecraft:repeating_command_block", 210},
			 {"minecraft:chain_command_block", 211},
			 {"minecraft:frosted_ice", 212},
			 {"minecraft:magma_block", 213},
			 {"minecraft:nether_wart_block", 214},
			 {"minecraft:red_nether_bricks", 215},
			 {"minecraft:bone_block", 216},
			 {"minecraft:structure_void", 217},
			 {"minecraft:observer", 218},
			 {"minecraft:white_shulker_box", 219},
			 {"minecraft:orange_shulker_box", 220},
			 {"minecraft:magenta_shulker_box", 221},
			 {"minecraft:light_blue_shulker_box", 222},
			 {"minecraft:yellow_shulker_box", 223},
			 {"minecraft:lime_shulker_box", 224},
			 {"minecraft:pink_shulker_box", 225},
			 {"minecraft:gray_shulker_box", 226},
			 {"minecraft:light_gray_shulker_box", 227},
			 {"minecraft:cyan_shulker_box", 228},
			 {"minecraft:purple_shulker_box", 229},
			 {"minecraft:blue_shulker_box", 230},
			 {"minecraft:brown_shulker_box", 231},
			 {"minecraft:green_shulker_box", 232},
			 {"minecraft:red_shulker_box", 233},
			 {"minecraft:black_shulker_box", 234},
			 {"minecraft:white_glazed_terracotta", 235},
			 {"minecraft:orange_glazed_terracotta", 236},
			 {"minecraft:magenta_glazed_terracotta", 237},
			 {"minecraft:light_blue_glazed_terracotta", 238},
			 {"minecraft:yellow_glazed_terracotta", 239},
			 {"minecraft:lime_glazed_terracotta", 240},
			 {"minecraft:pink_glazed_terracotta", 241},
			 {"minecraft:gray_glazed_terracotta", 242},
			 {"minecraft:light_gray_glazed_terracotta", 243},
			 {"minecraft:cyan_glazed_terracotta", 244},
			 {"minecraft:purple_glazed_terracotta", 245},
			 {"minecraft:blue_glazed_terracotta", 246},
			 {"minecraft:brown_glazed_terracotta", 247},
			 {"minecraft:green_glazed_terracotta", 248},
			 {"minecraft:red_glazed_terracotta", 249},
			 {"minecraft:black_glazed_terracotta", 250},
			 {"minecraft:gray_concrete", 251},
			 {"minecraft:gray_concrete_powder", 252},
			 {"minecraft:structure_block", 255}};

bool MCRFormat::parseData(SceneGraph &sceneGraph, io::ZipReadStream &stream, NamedBinaryTag &nbt, VoxelData &voxelData) {
	core_assert(nbt.id == TagId::LONG_ARRAY);
	core_assert(nbt.listItems < lengthof(voxelData.buf));
	voxelData.dataFound = nbt.listItems > 0;
	return stream.read(voxelData.buf, nbt.listItems * 8) == (int)nbt.listItems * 8;
}

bool MCRFormat::parsePalette(SceneGraph &sceneGraph, io::ZipReadStream &stream, NamedBinaryTag &nbt, VoxelData &voxelData) {
	const uint32_t items = nbt.listItems;
	const int compoundLevel = nbt.level;
	++nbt.level;
	voxelData.paletteFound = true;
	core::DynamicArray<uint32_t> idMapping(items);
	uint32_t paletteNum = 0;
	uint32_t numBlockIDs = lengthof(TYPES);
	for (uint32_t n = 0; n < items; ++n) {
		while (nbt.level >= compoundLevel) {
			wrapBool(getNext(stream, nbt));
			if (nbt.name == "Name" && nbt.id == TagId::STRING) {
				uint16_t nameLength;
				wrap(stream.readUInt16BE(nameLength));
				Log::debug("%*sLoad palette name of length %u", nbt.level * 3, " ", nameLength);

				core::String name;
				for (uint16_t i = 0u; i < nameLength; ++i) {
					uint8_t chr;
					wrap(stream.readUInt8(chr));
					name += (char)chr;
				}
				Log::debug("%*sPalette name: %s", nbt.level * 3, " ", name.c_str());
				idMapping[paletteNum] = 0;

				for (uint32_t id = 0; id < numBlockIDs; ++id) {
					if (TYPES[id].name != name) {
						continue;
					}
					idMapping[paletteNum] = id;
					break;
				}
				++paletteNum;
			} else {
				wrapBool(skip(stream, nbt, true));
			}
		}
		if (n != items - 1) {
			++nbt.level;
		}
	}
	--nbt.level;
	return true;
}

uint8_t MCRFormat::chunkVoxelColor(VoxelData &voxelData, int section, const glm::ivec3 &pos) {
	core_assert(0 <= pos.x && pos.x < MAX_SIZE);
	core_assert(0 <= pos.y && pos.y < MAX_SIZE);
	core_assert(0 <= pos.z && pos.z < MAX_SIZE);
	return voxelData.get(pos.x, pos.y, pos.z);
}

bool MCRFormat::parseBlockStates(SceneGraph &sceneGraph, io::ZipReadStream &stream, NamedBinaryTag &nbt, int y, int xPos, int zPos) {
	const int compoundLevel = nbt.level;

	VoxelData voxelData;
	while (nbt.level >= compoundLevel) {
		wrapBool(getNext(stream, nbt));
		if (nbt.name == "data" && nbt.id == TagId::LONG_ARRAY) {
			wrapBool(parseData(sceneGraph, stream, nbt, voxelData));
		} else if (nbt.name == "palette" && nbt.id == TagId::LIST) {
			wrapBool(parsePalette(sceneGraph, stream, nbt, voxelData));
		} else if (nbt.id == TagId::END) {
			break;
		} else {
			Log::debug("%*sunknown nbt '%s' (type %s)", nbt.level * 3, " ", nbt.name.c_str(), nbtTagNames[(int)nbt.listId]);
			wrapBool(skip(stream, nbt, true));
		}
	}

	if (!voxelData.dataFound) {
		return true;
	}

	const glm::ivec3 chunkSize(MAX_SIZE - 1);
	for (int section = 0; section < CHUNK_SECTIONS; ++section) {
		const glm::ivec3 chunkMins(xPos, (section - 128) * MAX_SIZE, zPos);
		const voxel::Region region(chunkMins, chunkMins + chunkSize);
		voxel::RawVolume *v = new voxel::RawVolume(region);
		glm::ivec3 sPos;
		for (sPos.y = 0; sPos.y <= chunkSize.y; ++sPos.y) {
			for (sPos.z = 0; sPos.z <= chunkSize.z; ++sPos.z) {
				for (sPos.x = 0; sPos.x <= chunkSize.x; ++sPos.x) {
					const uint8_t color = chunkVoxelColor(voxelData, section, sPos);
					if (color) {
						const uint8_t index = convertPaletteIndex(color);
						const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
						v->setVoxel(chunkMins.x + sPos.x, chunkMins.y + sPos.y, chunkMins.z + sPos.z, voxel);
					}
				}
			}
		}
		SceneGraphNode node;
		node.setVolume(v, true);
		sceneGraph.emplace(core::move(node));
	}
	return true;
}

bool MCRFormat::parseSections(SceneGraph &sceneGraph, io::ZipReadStream &stream, NamedBinaryTag &nbt, int xPos, int zPos) {
	const uint32_t items = nbt.listItems;
	const int compoundLevel = nbt.level;
	++nbt.level;
	const core::String name = nbt.name;
	Log::debug("Parse section with %i items ('%s')", items, name.c_str());
	for (uint32_t n = 0; n < items; ++n) {
		int8_t y = 0;
		while (nbt.level >= compoundLevel) {
			wrapBool(getNext(stream, nbt));
			if (nbt.name == "Y" && nbt.id == TagId::BYTE) {
				wrap(stream.readInt8(y));
				Log::debug("%*sSection y: %u", nbt.level * 3, " ", y);
			} else if (nbt.name == "block_states" && nbt.id == TagId::COMPOUND) {
				wrapBool(parseBlockStates(sceneGraph, stream, nbt, y, xPos, zPos));
			} else if (nbt.name == "biomes" && nbt.id == TagId::COMPOUND) {
				wrapBool(skip(stream, nbt, true));
			} else if (nbt.name == "BlockLight" && nbt.id == TagId::BYTE_ARRAY) {
				wrapBool(skip(stream, nbt, true));
			} else if (nbt.name == "SkyLight" && nbt.id == TagId::BYTE_ARRAY) {
				wrapBool(skip(stream, nbt, true));
			} else if (nbt.id == TagId::END) {
			} else {
				Log::debug("%*sunknown nbt '%s' (type %s)", nbt.level * 3, " ", nbt.name.c_str(), nbtTagNames[(int)nbt.listId]);
				wrapBool(skip(stream, nbt, true));
			}
		}
		if (n != items - 1) {
			++nbt.level;
		}
	}
	Log::debug("End section");
	--nbt.level;

	return true;
}

bool MCRFormat::parseNBTChunk(SceneGraph &sceneGraph, io::ZipReadStream &stream) {
	NamedBinaryTag nbt;
	wrapBool(getNext(stream, nbt));

	if (nbt.id != TagId::COMPOUND) {
		Log::error("Failed to read root compound");
		return false;
	}

	int32_t xPos = 0;
	int32_t zPos = 0;
	int32_t version = 0;

	while (!stream.eos()) {
		wrapBool(getNext(stream, nbt));
		if (nbt.name == "xPos" && nbt.id == TagId::INT) {
			wrap(stream.readInt32BE(xPos));
		} else if (nbt.name == "zPos" && nbt.id == TagId::INT) {
			wrap(stream.readInt32BE(zPos));
		} else if (nbt.name == "DataVersion" && nbt.id == TagId::INT) {
			wrap(stream.readInt32BE(version));
		} else if (nbt.name == "sections" && nbt.id == TagId::LIST) {
			return parseSections(sceneGraph, stream, nbt, xPos, zPos);
		} else {
			wrapBool(skip(stream, nbt, true));
		}
	}
	return true;
}

bool MCRFormat::skip(io::ZipReadStream &stream, NamedBinaryTag &nbt, bool marker) {
	switch (nbt.id) {
	case TagId::BYTE:
		Log::debug("%*sskip 1 byte ('%s')", nbt.level * 3, " ", nbt.name.c_str());
		if (stream.skip(1) != 1) {
			Log::error("Failed to skip 1 byte");
			return false;
		}
		break;
	case TagId::SHORT:
		Log::debug("%*sskip 2 bytes ('%s')", nbt.level * 3, " ", nbt.name.c_str());
		if (stream.skip(2) != 2) {
			Log::error("Failed to skip 2 bytes");
			return false;
		}
		break;
	case TagId::INT:
	case TagId::FLOAT:
		Log::debug("%*sskip 4 bytes ('%s')", nbt.level * 3, " ", nbt.name.c_str());
		if (stream.skip(4) != 4) {
			Log::error("Failed to skip 4 bytes");
			return false;
		}
		break;
	case TagId::LONG:
	case TagId::DOUBLE:
		Log::debug("%*sskip 8 bytes ('%s')", nbt.level * 3, " ", nbt.name.c_str());
		if (stream.skip(8) != 8) {
			Log::error("Failed to skip 8 bytes");
			return false;
		}
		break;
	case TagId::BYTE_ARRAY: {
		uint16_t length = nbt.listItems;
		Log::debug("%*sskip %u bytes ('%s')", nbt.level * 3, " ", length, nbt.name.c_str());
		if (stream.skip(length) != length) {
			Log::error("Failed to skip %u bytes", length);
			return false;
		}
		break;
	}
	case TagId::STRING: {
		uint16_t length;
		wrap(stream.readUInt16BE(length));
		core::String str;
		for (uint16_t i = 0u; i < length; ++i) {
			uint8_t chr;
			wrap(stream.readUInt8(chr));
			str += (char)chr;
		}
		Log::debug("%*s - skip %u bytes ('%s' = '%s')", nbt.level * 3, " ", length + 2, nbt.name.c_str(), str.c_str());
		break;
	}
	case TagId::INT_ARRAY: {
		uint32_t length = nbt.listItems * 4;
		Log::debug("%*sskip %u bytes ('%s')", nbt.level * 3, " ", length, nbt.name.c_str());
		if (stream.skip(length) != length) {
			Log::error("Failed to skip %u bytes", length);
			return false;
		}
		break;
	}
	case TagId::LONG_ARRAY: {
		uint32_t length = nbt.listItems * 8;
		Log::debug("%*sskip %u bytes ('%s')", nbt.level * 3, " ", length, nbt.name.c_str());
		if (stream.skip(length) != length) {
			Log::error("Failed to skip %u bytes", length);
			return false;
		}
		break;
	}
	case TagId::LIST: {
		++nbt.level;
		core_assert(nbt.level < 16);
		if (marker) {
			Log::debug("------------------start");
		}
		const uint32_t items = nbt.listItems;
		if (nbt.listId == TagId::COMPOUND) {
			const int compoundLevel = nbt.level;
			Log::debug("%*sskip %i compounds at level %i ('%s')", nbt.level * 3, " ", items, compoundLevel, nbt.name.c_str());
			for (uint32_t n = 0; n < items; ++n) {
				Log::debug("%*sskip compound %u", nbt.level * 3, " ", n);
				while (nbt.level >= compoundLevel) {
					wrapBool(getNext(stream, nbt));
					wrapBool(skip(stream, nbt, false));
				}
				if (n != items - 1) {
					++nbt.level;
				}
			}
		} else {
			// list of lists
			TagId listId = nbt.listId;
			if (listId == TagId::LIST) {
				if (stream.read(&nbt.id, sizeof(nbt.id)) != 1) {
					Log::debug("Failed to read type id");
					return false;
				}
				if (nbt.id == TagId::COMPOUND) {
					++nbt.level;
				}

				wrap(stream.readUInt32BE(nbt.listItems));
				Log::debug("%*sskip sublist %s with %i items of type %s", nbt.level * 3, " ", nbt.name.c_str(),
						   nbt.listItems, nbtTagNames[(int)nbt.id]);
				for (uint32_t n = 0; n < items; ++n) {
					Log::debug("%*sskip item %u", nbt.level * 3, " ", n);
					wrapBool(skip(stream, nbt, false));
				}
			} else {
				nbt.id = nbt.listId;

				Log::debug("%*sskip list %s at level %i ('%s') with %i items of type %s", nbt.level * 3, " ",
						nbt.name.c_str(), nbt.level, nbt.name.c_str(), items, nbtTagNames[(int)nbt.listId]);
				for (uint32_t n = 0; n < items; ++n) {
					Log::debug("%*sskip item %u", nbt.level * 3, " ", n);
					wrapBool(skip(stream, nbt, false));
				}
			}
		}
		if (marker) {
			Log::debug("------------------end--");
		}
		--nbt.level;
		break;
	}
	case TagId::COMPOUND: {
		const int compoundLevel = nbt.level;
		Log::debug("%*sskip compound %s at level %i ('%s')", nbt.level * 3, " ", nbt.name.c_str(), compoundLevel, nbt.name.c_str());
		if (marker) {
			Log::debug("------------------start");
		}
		while (nbt.level >= compoundLevel) {
			wrapBool(getNext(stream, nbt));
			wrapBool(skip(stream, nbt, false));
		}
		if (marker) {
			Log::debug("------------------end--");
		}
		break;
	}
	case TagId::END:
		break;
	default:
		Log::warn("Unknown tag %i ('%s')", (int)nbt.id, nbt.name.c_str());
		break;
	}
	return true;
}

bool MCRFormat::getNext(io::ReadStream &stream, MCRFormat::NamedBinaryTag &nbt) {
	static_assert(lengthof(nbtTagNames) == (int)TagId::MAX);
	if (stream.read(&nbt.id, sizeof(nbt.id)) != 1) {
		Log::debug("Failed to read type id");
		return false;
	}
	core_assert_msg(nbt.id >= TagId::END && nbt.id < TagId::MAX, "Found unknown id of type %s", nbtTagNames[(int)nbt.listId]);
	if (nbt.id == TagId::END) {
		--nbt.level;
		Log::debug("%*sFound compound end at level %i", nbt.level * 3, " ", nbt.level);
		return true;
	}
	uint16_t nameLength;
	wrap(stream.readUInt16BE(nameLength));
	Log::trace("Load string of length %u", nameLength);

	nbt.name.clear();
	for (uint16_t i = 0u; i < nameLength; ++i) {
		uint8_t chr;
		wrap(stream.readUInt8(chr));
		nbt.name += (char)chr;
	}
	Log::debug("%*sFound tag '%s' at level %i with type %s", nbt.level * 3, " ", nbt.name.c_str(), nbt.level, nbtTagNames[(int)nbt.id]);
	if (nbt.id == TagId::COMPOUND) {
		++nbt.level;
	}

	if (nbt.id == TagId::LIST) {
		if (stream.read(&nbt.listId, sizeof(nbt.listId)) != 1) {
			Log::debug("Failed to read type id");
			return false;
		}

		if (nbt.listId == TagId::COMPOUND) {
			++nbt.level;
		}

		wrap(stream.readUInt32BE(nbt.listItems));
		Log::debug("%*sFound list '%s' at level %i with sub type %s and %i entries", nbt.level * 3, " ", nbt.name.c_str(), nbt.level, nbtTagNames[(int)nbt.listId], nbt.listItems);
	} else if (nbt.id == TagId::LONG_ARRAY || nbt.id == TagId::INT_ARRAY || nbt.id == TagId::BYTE_ARRAY) {
		wrap(stream.readUInt32BE(nbt.listItems));
		Log::debug("%*sFound array '%s' at level %i with %i entries", nbt.level * 3, " ", nbt.name.c_str(), nbt.level, nbt.listItems);
	}

	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxel
