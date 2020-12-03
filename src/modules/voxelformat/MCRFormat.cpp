/**
 * @file
 */

#include "MCRFormat.h"
#include "SDL_endian.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/MemoryStreamReadOnly.h"
#include "core/Zip.h"
#include "core/collection/DynamicArray.h"
#include "io/File.h"

#include <glm/common.hpp>

namespace voxel {

#define wrap(expression)                                                                                               \
	do {                                                                                                               \
		if ((expression) != 0) {                                                                                       \
			Log::error("Could not load file: Not enough data in stream " CORE_STRINGIFY(                               \
						   #expression) " - still %i bytes left at " SDL_FILE ":%i",                                   \
					   (int)stream.remaining(), SDL_LINE);                                                             \
			return false;                                                                                              \
		}                                                                                                              \
	} while (0)

#define wrapBool(expression)                                                                                           \
	do {                                                                                                               \
		if (!(expression)) {                                                                                           \
			Log::error("Could not load file: Not enough data in stream " CORE_STRINGIFY(                               \
						   #expression) " - still %i bytes left at " SDL_FILE ":%i",                                   \
					   (int)stream.remaining(), SDL_LINE);                                                             \
			return false;                                                                                              \
		}                                                                                                              \
	} while (0)

bool MCRFormat::loadGroups(const io::FilePtr &file, VoxelVolumes &volumes) {
	if (!(bool)file || !file->exists()) {
		Log::error("Could not load mcr file: File doesn't exist");
		return false;
	}

	if (file->length() < SECTOR_BYTES) {
		Log::error("File does not contain enough data");
		return false;
	}

	core::String name = file->fileName().toLower();
	int chunkX = 0;
	int chunkZ = 0;
	char type = 'a';
	if (SDL_sscanf(name.c_str(), "r.%i.%i.mc%c", &chunkX, &chunkZ, &type) != 3) {
		Log::warn("Failed to parse the region chunk boundaries from filename %s", name.c_str());
		type = file->extension().last();
		chunkX = 0;
		chunkZ = 0;
	}

	io::FileStream stream(file.get());
	uint8_t *buffer = nullptr;
	const int length = file->read((void **)&buffer);
	if (length <= 0) {
		Log::error("Could not read the file into target buffer");
		return false;
	}
	switch (type) {
	case 'r':
	case 'a': {
		const int64_t fileSize = stream.remaining();
		if (fileSize <= 2 * SECTOR_BYTES) {
			Log::error("This region file has not enough data for the 8kb header");
			return false;
		}

		for (int i = 0; i < SECTOR_INTS; ++i) {
			union {
				uint8_t raw[4];
				struct Offset {
					uint32_t offset : 24;
					uint32_t sectors : 8;
				} offset;
			} data;
			for (int j = 0; j < 4; ++j) {
				wrap(stream.readByte(data.raw[j]));
			}
			_offsets[i].sectorCount = data.offset.sectors;
			const uint32_t value = data.offset.offset;
			_offsets[i].offset = SDL_SwapBE32(value) * SECTOR_BYTES;
		}

		for (int i = 0; i < SECTOR_INTS; ++i) {
			uint32_t lastModValue;
			wrap(stream.readIntBE(lastModValue));
			_chunkTimestamps[i] = lastModValue;
		}

		const bool success = loadMinecraftRegion(volumes, buffer, length, stream, chunkX, chunkZ);
		delete[] buffer;
		return success;
	}
	}
	delete[] buffer;
	Log::error("Unkown file type given: %c", type);
	return false;
}

bool MCRFormat::loadMinecraftRegion(VoxelVolumes &volumes, const uint8_t *buffer, int length, io::FileStream &stream, int chunkX, int chunkZ) {
	for (int i = 0; i < SECTOR_INTS; ++i) {
		if (_offsets[i].sectorCount == 0 || _offsets[i].offset == 0) {
			continue;
		}
		if (_offsets[i].offset >= (uint32_t)length) {
			Log::error("Exceeded stream boundaries: %u, %i", _offsets[i].offset, length);
			return false;
		}
		wrap(stream.seek(_offsets[i].offset));
		if (!readCompressedNBT(volumes, buffer, length, stream)) {
			Log::error("Failed to load minecraft chunk section %i for offset %u", i, _offsets[i].offset);
			return false;
		}
	}

	return true;
}

bool MCRFormat::readCompressedNBT(VoxelVolumes &volumes, const uint8_t *buffer, int length, io::FileStream &stream) {
	uint32_t nbtSize;
	wrap(stream.readIntBE(nbtSize));
	if (nbtSize == 0) {
		Log::debug("Empty nbt chunk found");
		return true;
	}

	if (nbtSize > 0x1FFFFFF) {
		Log::error("Size of nbt data exceeds the max allowed value: %u", nbtSize);
		return false;
	}

	uint8_t version;
	wrap(stream.readByte(version));
	if (version != VERSION_GZIP && version != VERSION_DEFLATE) {
		Log::error("Unsupported version found: %u", version);
		return false;
	}

	// the version is included in the length
	--nbtSize;
	const uint32_t sizeHint = nbtSize * 50; // TODO: improve this
	uint8_t *nbtData = new uint8_t[sizeHint];
	size_t finalBufSize;
	if (!core::zip::uncompress(buffer + stream.pos(), nbtSize, nbtData, sizeHint, &finalBufSize)) {
		delete[] nbtData;
		Log::error("Failed to uncompress nbt data of compressed size %u", nbtSize);
		return false;
	}

	const bool success = parseNBTChunk(volumes, nbtData, finalBufSize);
	delete[] nbtData;
	return success;
}

// this list was found in enkiMI by Doug Binks
static const struct {
	const core::String name;
	const uint8_t id;
} TYPES[] = {
	{ "minecraft:air", 0 },
	{ "minecraft:stone", 1 },
	{ "minecraft:andesite", 1 },
	{ "minecraft:seagrass", 2 },
	{ "minecraft:grass", 2 },
	{ "minecraft:grass_block", 2 },
	{ "minecraft:dirt", 3 },
	{ "minecraft:cobblestone", 4 },
	{ "minecraft:oak_planks", 5 },
	{ "minecraft:oak_sapling", 6 },
	{ "minecraft:bedrock", 7 },
	{ "minecraft:flowing_water", 8 },
	{ "minecraft:water", 9 },
	{ "minecraft:flowing_lava", 10 },
	{ "minecraft:lava", 11 },
	{ "minecraft:sand", 12 },
	{ "minecraft:gravel", 13 },
	{ "minecraft:gold_ore", 14 },
	{ "minecraft:iron_ore", 15 },
	{ "minecraft:coal_ore", 16 },
	{ "minecraft:oak_log", 17 },
	{ "minecraft:acacia_leaves", 18 },
	{ "minecraft:spruce_leaves", 18 },
	{ "minecraft:oak_leaves", 18 },
	{ "minecraft:jungle_leaves", 18 },
	{ "minecraft:birch_leaves", 18 },
	{ "minecraft:sponge", 19 },
	{ "minecraft:glass", 20 },
	{ "minecraft:lapis_ore", 21 },
	{ "minecraft:lapis_block", 22 },
	{ "minecraft:dispenser", 23 },
	{ "minecraft:sandstone", 24 },
	{ "minecraft:note_block", 25 },
	{ "minecraft:red_bed", 26 },
	{ "minecraft:powered_rail", 27 },
	{ "minecraft:detector_rail", 28 },
	{ "minecraft:sticky_piston", 29 },
	{ "minecraft:cobweb", 30 },
	{ "minecraft:tall_grass", 31 },
	{ "minecraft:tall_seagrass", 31 },
	{ "minecraft:dead_bush", 32 },
	{ "minecraft:piston", 33 },
	{ "minecraft:piston_head", 34 },
	{ "minecraft:white_concrete", 35 },
	{ "minecraft:dandelion", 37 },
	{ "minecraft:poppy", 38 },
	{ "minecraft:brown_mushroom", 39 },
	{ "minecraft:red_mushroom", 40 },
	{ "minecraft:gold_block", 41 },
	{ "minecraft:iron_block", 42 },
	{ "minecraft:smooth_stone_slab", 43 },
	{ "minecraft:stone_slab", 44 },
	{ "minecraft:brick_wall", 45 },
	{ "minecraft:bricks", 45 },
	{ "minecraft:tnt", 46 },
	{ "minecraft:bookshelf", 47 },
	{ "minecraft:mossy_cobblestone", 48 },
	{ "minecraft:obsidian", 49 },
	{ "minecraft:torch", 50 },
	{ "minecraft:fire", 51 },
	{ "minecraft:spawner", 52 },
	{ "minecraft:oak_stairs", 53 },
	{ "minecraft:chest", 54 },
	{ "minecraft:redstone_wire", 55 },
	{ "minecraft:diamond_ore", 56 },
	{ "minecraft:diamond_block", 57 },
	{ "minecraft:crafting_table", 58 },
	{ "minecraft:wheat", 59 },
	{ "minecraft:farmland", 60 },
	{ "minecraft:furnace", 61 },
	{ "minecraft:campfire", 62 },
	{ "minecraft:oak_sign", 63 },
	{ "minecraft:oak_door", 64 },
	{ "minecraft:ladder", 65 },
	{ "minecraft:rail", 66 },
	{ "minecraft:stone_stairs", 67 },
	{ "minecraft:oak_wall_sign", 68 },
	{ "minecraft:lever", 69 },
	{ "minecraft:stone_pressure_plate", 70 },
	{ "minecraft:iron_door", 71 },
	{ "minecraft:oak_pressure_plate", 72 },
	{ "minecraft:redstone_ore", 73 },
	{ "minecraft:red_concrete", 74 },
	{ "minecraft:redstone_wall_torch", 75 },
	{ "minecraft:redstone_torch", 76 },
	{ "minecraft:stone_button", 77 },
	{ "minecraft:snow_block", 78 },
	{ "minecraft:ice", 79 },
	{ "minecraft:snow", 80 },
	{ "minecraft:cactus", 81 },
	{ "minecraft:clay", 82 },
	{ "minecraft:bamboo", 83 },
	{ "minecraft:jukebox", 84 },
	{ "minecraft:oak_fence", 85 },
	{ "minecraft:pumpkin", 86 },
	{ "minecraft:netherrack", 87 },
	{ "minecraft:soul_sand", 88 },
	{ "minecraft:glowstone", 89 },
	{ "minecraft:portal", 90 },
	{ "minecraft:carved_pumpkin", 91 },
	{ "minecraft:cake", 92 },
	{ "minecraft:repeater", 93 },
	{ "minecraft:skeleton_skull", 94 },
	{ "minecraft:white_stained_glass", 95 },
	{ "minecraft:oak_trapdoor", 96 },
	{ "minecraft:turtle_egg", 97 },
	{ "minecraft:stone_bricks", 98 },
	{ "minecraft:brown_mushroom_block", 99 },
	{ "minecraft:red_mushroom_block", 100 },
	{ "minecraft:iron_bars", 101 },
	{ "minecraft:light_blue_stained_glass_pane", 102 },
	{ "minecraft:melon", 103 },
	{ "minecraft:pumpkin_stem", 104 },
	{ "minecraft:melon_stem", 105 },
	{ "minecraft:vine", 106 },
	{ "minecraft:oak_fence_gate", 107 },
	{ "minecraft:brick_stairs", 108 },
	{ "minecraft:stone_brick_stairs", 109 },
	{ "minecraft:mycelium", 110 },
	{ "minecraft:light_gray_concrete", 111 },
	{ "minecraft:nether_brick", 112 },
	{ "minecraft:nether_brick_fence", 113 },
	{ "minecraft:nether_brick_stairs", 114 },
	{ "minecraft:nether_wart", 115 },
	{ "minecraft:enchanting_table", 116 },
	{ "minecraft:brewing_stand", 117 },
	{ "minecraft:cauldron", 118 },
	{ "minecraft:end_portal", 119 },
	{ "minecraft:end_portal_frame", 120 },
	{ "minecraft:end_stone", 121 },
	{ "minecraft:dragon_egg", 122 },
	{ "minecraft:redstone_lamp", 123 },
	{ "minecraft:shroomlight", 124 },
	{ "minecraft:oak_wood", 125 },
	{ "minecraft:oak_slab", 126 },
	{ "minecraft:cocoa", 127 },
	{ "minecraft:sandstone_stairs", 128 },
	{ "minecraft:emerald_ore", 129 },
	{ "minecraft:ender_chest", 130 },
	{ "minecraft:tripwire_hook", 131 },
	{ "minecraft:tripwire", 132 },
	{ "minecraft:emerald_block", 133 },
	{ "minecraft:spruce_stairs", 134 },
	{ "minecraft:birch_stairs", 135 },
	{ "minecraft:jungle_stairs", 136 },
	{ "minecraft:command_block", 137 },
	{ "minecraft:beacon", 138 },
	{ "minecraft:cobblestone_wall", 139 },
	{ "minecraft:flower_pot", 140 },
	{ "minecraft:carrots", 141 },
	{ "minecraft:potatoes", 142 },
	{ "minecraft:oak_button", 143 },
	{ "minecraft:skeleton_wall_skull", 144 },
	{ "minecraft:anvil", 145 },
	{ "minecraft:trapped_chest", 146 },
	{ "minecraft:light_weighted_pressure_plate", 147 },
	{ "minecraft:heavy_weighted_pressure_plate", 148 },
	{ "minecraft:comparator", 149 },
	{ "minecraft:chain", 150 },
	{ "minecraft:daylight_detector", 151 },
	{ "minecraft:redstone_block", 152 },
	{ "minecraft:nether_quartz_ore", 153 },
	{ "minecraft:hopper", 154 },
	{ "minecraft:quartz_block", 155 },
	{ "minecraft:quartz_stairs", 156 },
	{ "minecraft:activator_rail", 157 },
	{ "minecraft:dropper", 158 },
	{ "minecraft:pink_stained_glass", 159 },
	{ "minecraft:white_stained_glass_pane", 160 },
	{ "minecraft:dead_brain_coral", 161 },
	{ "minecraft:acacia_planks", 162 },
	{ "minecraft:acacia_stairs", 163 },
	{ "minecraft:dark_oak_stairs", 164 },
	{ "minecraft:slime_block", 165 },
	{ "minecraft:barrier", 166 },
	{ "minecraft:iron_trapdoor", 167 },
	{ "minecraft:prismarine", 168 },
	{ "minecraft:sea_lantern", 169 },
	{ "minecraft:hay_block", 170 },
	{ "minecraft:white_carpet", 171 },
	{ "minecraft:coarse_dirt", 172 },
	{ "minecraft:coal_block", 173 },
	{ "minecraft:packed_ice", 174 },
	{ "minecraft:orange_concrete", 175 },
	{ "minecraft:white_banner", 176 },
	{ "minecraft:white_wall_banner", 177 },
	{ "minecraft:white_concrete_powder", 178 },
	{ "minecraft:red_sandstone", 179 },
	{ "minecraft:red_sandstone_stairs", 180 },
	{ "minecraft:red_sandstone_wall", 181 },
	{ "minecraft:red_sandstone_slab", 182 },
	{ "minecraft:spruce_fence_gate", 183 },
	{ "minecraft:birch_fence_gate", 184 },
	{ "minecraft:jungle_fence_gate", 185 },
	{ "minecraft:dark_oak_fence_gate", 186 },
	{ "minecraft:acacia_fence_gate", 187 },
	{ "minecraft:spruce_fence", 188 },
	{ "minecraft:birch_fence", 189 },
	{ "minecraft:jungle_fence", 190 },
	{ "minecraft:dark_oak_fence", 191 },
	{ "minecraft:acacia_fence", 192 },
	{ "minecraft:spruce_door", 193 },
	{ "minecraft:birch_door", 194 },
	{ "minecraft:jungle_door", 195 },
	{ "minecraft:acacia_door", 196 },
	{ "minecraft:dark_oak_door", 197 },
	{ "minecraft:end_rod", 198 },
	{ "minecraft:chorus_plant", 199 },
	{ "minecraft:chorus_flower", 200 },
	{ "minecraft:purpur_block", 201 },
	{ "minecraft:purpur_pillar", 202 },
	{ "minecraft:purpur_stairs", 203 },
	{ "minecraft:purple_stained_glass", 204 },
	{ "minecraft:purpur_slab", 205 },
	{ "minecraft:end_stone_bricks", 206 },
	{ "minecraft:beetroots", 207 },
	{ "minecraft:grass_path", 208 },
	{ "minecraft:end_gateway", 209 },
	{ "minecraft:repeating_command_block", 210 },
	{ "minecraft:chain_command_block", 211 },
	{ "minecraft:frosted_ice", 212 },
	{ "minecraft:magma_block", 213 },
	{ "minecraft:nether_wart_block", 214 },
	{ "minecraft:red_nether_bricks", 215 },
	{ "minecraft:bone_block", 216 },
	{ "minecraft:structure_void", 217 },
	{ "minecraft:observer", 218 },
	{ "minecraft:white_shulker_box", 219 },
	{ "minecraft:orange_shulker_box", 220 },
	{ "minecraft:magenta_shulker_box", 221 },
	{ "minecraft:light_blue_shulker_box", 222 },
	{ "minecraft:yellow_shulker_box", 223 },
	{ "minecraft:lime_shulker_box", 224 },
	{ "minecraft:pink_shulker_box", 225 },
	{ "minecraft:gray_shulker_box", 226 },
	{ "minecraft:light_gray_shulker_box", 227 },
	{ "minecraft:cyan_shulker_box", 228 },
	{ "minecraft:purple_shulker_box", 229 },
	{ "minecraft:blue_shulker_box", 230 },
	{ "minecraft:brown_shulker_box", 231 },
	{ "minecraft:green_shulker_box", 232 },
	{ "minecraft:red_shulker_box", 233 },
	{ "minecraft:black_shulker_box", 234 },
	{ "minecraft:white_glazed_terracotta", 235 },
	{ "minecraft:orange_glazed_terracotta", 236 },
	{ "minecraft:magenta_glazed_terracotta", 237 },
	{ "minecraft:light_blue_glazed_terracotta", 238 },
	{ "minecraft:yellow_glazed_terracotta", 239 },
	{ "minecraft:lime_glazed_terracotta", 240 },
	{ "minecraft:pink_glazed_terracotta", 241 },
	{ "minecraft:gray_glazed_terracotta", 242 },
	{ "minecraft:light_gray_glazed_terracotta", 243 },
	{ "minecraft:cyan_glazed_terracotta", 244 },
	{ "minecraft:purple_glazed_terracotta", 245 },
	{ "minecraft:blue_glazed_terracotta", 246 },
	{ "minecraft:brown_glazed_terracotta", 247 },
	{ "minecraft:green_glazed_terracotta", 248 },
	{ "minecraft:red_glazed_terracotta", 249 },
	{ "minecraft:black_glazed_terracotta", 250 },
	{ "minecraft:gray_concrete", 251 },
	{ "minecraft:gray_concrete_powder", 252 },
	{ "minecraft:structure_block", 255 }
};

bool MCRFormat::parseNBTChunk(VoxelVolumes &volumes, const uint8_t *buffer, int length) {
	core::MemoryStreamReadOnly stream(buffer, length);

	MCRFormat::NamedBinaryTag root;
	wrapBool(getNext(stream, root));

	if (root.id != TagId::COMPOUND) {
		Log::error("Failed to read root compound");
		return false;
	}

	int32_t xPos = 0;
	int32_t zPos = 0;
	const uint8_t *sectionPointers[16];

	MCRFormat::NamedBinaryTag nbt;
	while (!stream.eos()) {
		wrapBool(getNext(stream, nbt));
		Log::debug("Name: %s", nbt.name.c_str());
		if (nbt.name == "Level") {
			while (!stream.eos()) {
				wrapBool(getNext(stream, nbt));
				if (nbt.name == "xPos" && nbt.id == TagId::INT) {
					wrap(stream.readIntBE((uint32_t &)xPos));
				} else if (nbt.name == "zPos" && nbt.id == TagId::INT) {
					wrap(stream.readIntBE((uint32_t &)zPos));
				} else if (nbt.name == "Sections" && nbt.id == TagId::LIST) {
					TagId listId;
					uint32_t sections;
					wrap(stream.read(listId));
					if (listId != TagId::COMPOUND) {
						Log::error("Unexpected section tag id: %i", (int)listId);
						return false;
					}
					wrap(stream.readIntBE(sections));
					Log::debug("Found %u Sections (type: %i)", sections, (int)listId);

					for (uint32_t i = 0; i < sections; ++i) {
						int expectedLevel = nbt.level - 1;
						uint8_t y = 0;
						const uint8_t *blockStates = nullptr;
						const uint8_t *blocks = nullptr;
						while (!stream.eos()) {
							wrapBool(getNext(stream, nbt));
							if (nbt.id == TagId::END) {
								if (nbt.level == expectedLevel) {
									Log::debug("Found section end");
									break;
								}
								Log::debug("Found sub compound, continue the loop at level %i", nbt.level);
							}
							if (nbt.name == "Y" && nbt.id == TagId::BYTE) {
								wrap(stream.read(y));
								if (y >= lengthof(sectionPointers)) {
									Log::error("section y value exceeds the max allowed value of 15: %u", y);
									return false;
								}
								Log::debug("Section y: %u", y);
							} else if (nbt.name == "BlockStates" && nbt.id == TagId::LONG_ARRAY) {
								uint32_t arrayLength;
								wrap(stream.readIntBE(arrayLength));
								blockStates = buffer + stream.pos();
								Log::debug("Found %u blockstates, %u", arrayLength, (uint32_t)stream.remaining());
								wrapBool(stream.skip(arrayLength * 8));
							} else if (nbt.name == "Palette" && nbt.id == TagId::LIST) {
								TagId paletteListId;
								uint32_t palettes;
								wrap(stream.read(paletteListId));
								if (paletteListId != TagId::COMPOUND) {
									Log::error("Unexpected palette tag id: %i", (int)paletteListId);
									return false;
								}
								wrap(stream.readIntBE(palettes));
								Log::debug("Found %u palettes (type: %i)", palettes, (int)paletteListId);

								core::DynamicArray<uint32_t> idMapping(palettes);
								uint32_t numBlockIDs = lengthof(TYPES);

								uint32_t paletteNum = 0;
								for (uint32_t p = 0; p < palettes; ++p) {
									wrapBool(getNext(stream, nbt));
									if (nbt.name == "Name" && nbt.id == TagId::STRING) {
										uint16_t nameLength;
										wrap(stream.readShortBE(nameLength));
										Log::debug("Load palette name of length %u", nameLength);

										core::String name;
										for (uint16_t i = 0u; i < nameLength; ++i) {
											uint8_t chr;
											wrap(stream.readByte(chr));
											name += chr;
										}
										Log::debug("Palette name: %s", name.c_str());
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
										wrapBool(skip(stream, nbt.id));
									}
								}
							} else if (nbt.name == "Blocks" && nbt.id == TagId::LONG_ARRAY) {
								uint32_t arrayLength;
								wrap(stream.readIntBE(arrayLength));
								Log::debug("Found %u Blocks, %u", arrayLength, (uint32_t)stream.remaining());

								blocks = buffer + stream.pos();
								Log::debug("Found %u blockstates, %u", arrayLength, (uint32_t)stream.remaining());
								wrapBool(stream.skip(arrayLength * 8));
							} else {
								Log::debug("Skip %s", nbt.name.c_str());
								wrapBool(skip(stream, nbt.id));
							}
						}
						if (blocks != nullptr) {
							sectionPointers[y] = blocks;
						}
						if (blockStates) {
							sectionPointers[y] = blockStates;
						}
					}
				} else if (nbt.name == "Heightmaps" && nbt.id == TagId::COMPOUND) {
					int expectedLevel = nbt.level - 1;
					while (getNext(stream, nbt)) {
						if (nbt.id == TagId::END && nbt.level == expectedLevel) {
							break;
						}
						wrapBool(skip(stream, nbt.id));
					}
				} else if ((nbt.name == "TileTicks" || nbt.name == "ToBeTicked" || nbt.name == "TileEntities" ||
							nbt.name == "Entities") &&
						   nbt.id == TagId::LIST) {
					wrapBool(skip(stream, nbt.id));
				} else if (nbt.name == "Status" && nbt.id == TagId::STRING) {
					wrapBool(skip(stream, nbt.id));
				} else if (nbt.name == "LastUpdate" && nbt.id == TagId::LONG) {
					wrapBool(skip(stream, nbt.id));
				} else if (nbt.name == "InhabitedTime" && nbt.id == TagId::LONG) {
					wrapBool(skip(stream, nbt.id));
				} else if (nbt.name == "Biomes" && nbt.id == TagId::INT_ARRAY) {
					wrapBool(skip(stream, nbt.id));
				} else {
					Log::trace("skip %s: %u", nbt.name.c_str(), (uint32_t)stream.remaining());
					wrapBool(skip(stream, nbt.id));
					Log::trace("after skip %s: %u", nbt.name.c_str(), (uint32_t)stream.remaining());
				}
			}
		} else {
			Log::trace("skip %s: %u", nbt.name.c_str(), (uint32_t)stream.remaining());
			wrapBool(skip(stream, nbt.id));
			Log::trace("after skip %s: %u", nbt.name.c_str(), (uint32_t)stream.remaining());
		}
	}
	return true;
}

bool MCRFormat::skip(core::MemoryStreamReadOnly &stream, TagId id) {
	switch (id) {
	case TagId::BYTE:
		Log::debug("skip 1 byte");
		wrapBool(stream.skip(1));
		break;
	case TagId::SHORT:
		Log::debug("skip 2 bytes");
		wrapBool(stream.skip(2));
		break;
	case TagId::INT:
	case TagId::FLOAT:
		Log::debug("skip 4 bytes");
		wrapBool(stream.skip(4));
		break;
	case TagId::LONG:
	case TagId::DOUBLE:
		Log::debug("skip 8 bytes");
		wrapBool(stream.skip(8));
		break;
	case TagId::BYTE_ARRAY: {
		uint32_t length;
		wrap(stream.readIntBE(length));
		Log::debug("skip %u bytes", length + 4);
		wrapBool(stream.skip(length));
		break;
	}
	case TagId::STRING: {
		uint16_t length;
		wrap(stream.readShortBE(length));
		Log::debug("skip %u bytes", length + 2);
		wrapBool(stream.skip(length));
		break;
	}
	case TagId::INT_ARRAY: {
		uint32_t length;
		wrap(stream.readIntBE(length));
		Log::debug("skip %u bytes", length * 4 + 4);
		wrapBool(stream.skip(length * 4));
		break;
	}
	case TagId::LONG_ARRAY: {
		uint32_t length;
		wrap(stream.readIntBE(length));
		Log::debug("skip %u bytes", length * 8 + 4);
		wrapBool(stream.skip(length * 8));
		break;
	}
	case TagId::LIST: {
		TagId listId;
		uint32_t length;
		wrap(stream.read(listId));
		wrap(stream.readIntBE(length));
		Log::debug("skip 5 bytes + %u list elements following", length);
		for (uint32_t i = 0; i < length; ++i) {
			skip(stream, listId);
		}
		break;
	}
	case TagId::COMPOUND: {
		MCRFormat::NamedBinaryTag compound;
		compound.level = 1;
		const uint32_t pos = stream.pos();
		Log::debug("skip compound");
		while (compound.level > 0) {
			wrapBool(getNext(stream, compound));
			wrapBool(skip(stream, compound.id));
		}
		Log::debug("skip compound end: %u bytes", stream.pos() - pos);
		break;
	}
	case TagId::END:
		break;
	default:
		Log::warn("Unknown tag %i", (int)id);
		break;
	}
	return true;
}

bool MCRFormat::getNext(core::MemoryStreamReadOnly &stream, MCRFormat::NamedBinaryTag &nbt) {
	wrap(stream.read(nbt.id));
	if (nbt.id == TagId::END) {
		Log::debug("Found compound end at level %i", nbt.level);
		--nbt.level;
		return true;
	}
	uint16_t nameLength;
	wrap(stream.readShortBE(nameLength));
	Log::debug("Load string of length %u", nameLength);

	nbt.name.clear();
	for (uint16_t i = 0u; i < nameLength; ++i) {
		uint8_t chr;
		wrap(stream.readByte(chr));
		nbt.name += chr;
	}
	Log::debug("Found tag %s at level %i", nbt.name.c_str(), nbt.level);
	if (nbt.id == TagId::COMPOUND) {
		++nbt.level;
	}
	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxel
