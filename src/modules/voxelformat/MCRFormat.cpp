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
#include "private/NamedBinaryTag.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolumeWrapper.h"
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

bool MCRFormat::loadGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
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

	_palette.minecraft();
	const voxel::Palette &palette = voxel::getPalette();
	for (size_t i = 0; i < _palette.size(); ++i) {
		_paletteMapping[i] = palette.getClosestMatch(core::Color::fromRGBA(_palette.colors[i]));
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
		}

		const bool success = loadMinecraftRegion(sceneGraph, stream);
		return success;
	}
	}
	Log::error("Unkown file type given: %c", type);
	return false;
}

bool MCRFormat::loadMinecraftRegion(SceneGraph &sceneGraph, io::SeekableReadStream &stream) {
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
		if (!readCompressedNBT(sceneGraph, stream, i)) {
			Log::error("Failed to load minecraft chunk section %i for offset %u", i, (int)_offsets[i].offset);
			return false;
		}
	}

	return true;
}

bool MCRFormat::readCompressedNBT(SceneGraph &sceneGraph, io::SeekableReadStream &stream, int sector) {
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
	const int32_t dataVersion = root.get("DataVersion").int32();
	if (dataVersion >= 2844) {
		volume = parseSections(dataVersion, root, sector);
		if (volume == nullptr) {
			return false;
		}
	} else {
		volume = parseLevelCompound(dataVersion, root, sector);
		if (volume == nullptr) {
			return false;
		}
	}
	SceneGraphNode node(SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	sceneGraph.emplace(core::move(node));
	return true;
}

int MCRFormat::getVoxel(int dataVersion, const priv::NamedBinaryTag &data, const glm::ivec3 &pos) {
	const uint32_t i = pos.y * MAX_SIZE * MAX_SIZE + pos.z * MAX_SIZE + pos.x;
	if (data.type() != priv::TagType::BYTE_ARRAY) {
		Log::error("Unknown block data type: %i for version %i", (int)data.type(), dataVersion);
		return -1;
	}
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

int MCRFormat::getVoxel(int dataVersion, const MinecraftSectionPalette &secPal, const priv::NamedBinaryTag &data,
							const glm::ivec3 &pos) {
	if (data.type() != priv::TagType::LONG_ARRAY) {
		Log::error("Unknown block data type: %i for version %i", (int)data.type(), dataVersion);
		return -1;
	}
	const uint32_t i = pos.y * MAX_SIZE * MAX_SIZE + pos.z * MAX_SIZE + pos.x;
	const uint32_t bitsPerBlock = secPal.numBits;
	const uint32_t blocksPerLong = 64 / secPal.numBits;
	const uint64_t mask = UINT64_MAX >> (64 - secPal.numBits);
	const core::DynamicArray<int64_t> &blocks = *data.longArray();
	const size_t idx = i / blocksPerLong;
	if (idx >= blocks.size()) {
		// TODO: 1.13 file triggers this sanity check
		Log::error("Long array index out of bounds: %i/%i", (int)idx, (int)data.longArray()->size());
		return -1;
	}
	const uint64_t blockIndex = (blocks[idx] >> (bitsPerBlock * (i % blocksPerLong))) & mask;
	if (blockIndex < secPal.pal.size()) {
		return (int)secPal.pal[blockIndex];
	}
	return 0;
}

voxel::RawVolume *MCRFormat::error(SectionVolumes &volumes) {
	for (voxel::RawVolume *v : volumes) {
		delete v;
	}
	return nullptr;
}

voxel::RawVolume* MCRFormat::finalize(SectionVolumes& volumes, int xPos, int zPos) {
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

bool MCRFormat::parseBlockStates(int dataVersion, const priv::NamedBinaryTag &data, SectionVolumes &volumes, int sectionY, const MinecraftSectionPalette &secPal) {
	const bool hasData = data.type() == priv::TagType::LONG_ARRAY && !data.longArray()->empty();

	constexpr glm::ivec3 mins(0, 0, 0);
	constexpr glm::ivec3 maxs(MAX_SIZE - 1, MAX_SIZE - 1, MAX_SIZE - 1);
	const voxel::Region region(mins, maxs);
	voxel::RawVolume *v = new voxel::RawVolume(region);
	voxel::RawVolumeWrapper wrapper(v);

	if (secPal.pal.empty()) {
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
						const uint8_t index = convertPaletteIndex(color);
						const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
						wrapper.setVoxel(sPos, voxel);
					}
				}
			}
		}
	} else if (hasData) {
		glm::ivec3 sPos;
		for (sPos.y = 0; sPos.y < MAX_SIZE; ++sPos.y) {
			for (sPos.z = 0; sPos.z < MAX_SIZE; ++sPos.z) {
				for (sPos.x = 0; sPos.x < MAX_SIZE; ++sPos.x) {
					const int color = getVoxel(dataVersion, secPal, data, sPos);
					if (color < 0) {
						Log::error("Failed to load voxel at position %i:%i:%i", sPos.x, sPos.y, sPos.z);
						delete v;
						return false;
					}
					if (color) {
						const uint8_t index = convertPaletteIndex(color);
						const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
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

voxel::RawVolume *MCRFormat::parseSections(int dataVersion, const priv::NamedBinaryTag &root, int sector) {
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

	const priv::NBTList *sectionsList = sections.list();
	if (sectionsList == nullptr) {
		Log::error("Could not find 'sections' entries");
		return nullptr;
	}
	SectionVolumes volumes;
	for (const priv::NamedBinaryTag &section : *sectionsList) {
		const priv::NamedBinaryTag &blockStates = section.get("block_states");
		if (!blockStates.valid()) {
			Log::error("Could not find 'block_states'");
			return error(volumes);
		}
		const int8_t sectionY = section.get("Y").int8();

		const priv::NamedBinaryTag &palette = blockStates.get("palette");
		if (!palette.valid()) {
			Log::error("Could not find 'palette'");
			return error(volumes);
		}
		MinecraftSectionPalette secPal;
		if (!parsePaletteList(dataVersion, palette, secPal)) {
			Log::error("Could not parse palette chunk");
			return error(volumes);
		}
		const priv::NamedBinaryTag &data = blockStates.get("data");
		if (!parseBlockStates(dataVersion, data, volumes, sectionY, secPal)) {
			Log::error("Failed to parse 'data' tag");
			return error(volumes);
		}
	}
	return finalize(volumes, xPos, zPos);
}

voxel::RawVolume *MCRFormat::parseLevelCompound(int dataVersion, const priv::NamedBinaryTag &root, int sector) {
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

	const priv::NamedBinaryTag &sections = levels.get("Sections");
	if (!sections.valid()) {
		Log::error("Could not find 'Sections' tag");
		return nullptr;
	}
	if (sections.type() != priv::TagType::LIST) {
		Log::error("Invalid type for 'Sections' tag: %i", (int)sections.type());
		return nullptr;
	}
	SectionVolumes volumes;
	const priv::NBTList &sectionsList = *sections.list();
	for (const priv::NamedBinaryTag &section : sectionsList) {
		const int8_t sectionY = section.get("Y").int8();
		MinecraftSectionPalette secPal;
		const priv::NamedBinaryTag &palette = section.get("Palette");
		if (palette.valid()) {
			if (!parsePaletteList(dataVersion, palette, secPal)) {
				Log::error("Failed to parse 'Palette' tag");
				return error(volumes);
			}
		}

		// TODO:"Data"(byte_array)
		//const priv::NamedBinaryTag &data = section.get("Data");
		const core::String &tagId = dataVersion <= 1343 ? "Blocks" : "BlockStates";
		const priv::NamedBinaryTag &blockStates = section.get(tagId);
		if (!blockStates.valid()) {
			Log::error("Could not find '%s'", tagId.c_str());
			return error(volumes);
		}
		if (!parseBlockStates(dataVersion, blockStates, volumes, sectionY, secPal)) {
			Log::error("Failed to parse '%s' tag", tagId.c_str());
			return error(volumes);
		}
	}
	return finalize(volumes, xPos, zPos);
}

using PaletteMap = core::StringMap<int>;

// this list was found in enkiMI by Doug Binks and extended
static const PaletteMap &getPaletteMap() {
	static const PaletteMap mcPalette{{"air", 0},
									  {"stone", 1},
									  {"andesite", 1},
									  {"seagrass", 2},
									  {"grass", 2},
									  {"grass_block", 2},
									  {"dirt", 3},
									  {"cobblestone", 4},
									  {"oak_planks", 5},
									  {"oak_sapling", 6},
									  {"bedrock", 7},
									  {"flowing_water", 8},
									  {"water", 9},
									  {"flowing_lava", 10},
									  {"lava", 11},
									  {"sand", 12},
									  {"gravel", 13},
									  {"gold_ore", 14},
									  {"iron_ore", 15},
									  {"coal_ore", 16},
									  {"oak_log", 17},
									  {"acacia_leaves", 18},
									  {"spruce_leaves", 18},
									  {"oak_leaves", 18},
									  {"jungle_leaves", 18},
									  {"birch_leaves", 18},
									  {"sponge", 19},
									  {"glass", 20},
									  {"lapis_ore", 21},
									  {"lapis_block", 22},
									  {"dispenser", 23},
									  {"sandstone", 24},
									  {"note_block", 25},
									  {"red_bed", 26},
									  {"powered_rail", 27},
									  {"detector_rail", 28},
									  {"sticky_piston", 29},
									  {"cobweb", 30},
									  {"tall_grass", 31},
									  {"tall_seagrass", 31},
									  {"dead_bush", 32},
									  {"piston", 33},
									  {"piston_head", 34},
									  {"white_concrete", 35},
									  {"dandelion", 37},
									  {"poppy", 38},
									  {"brown_mushroom", 39},
									  {"red_mushroom", 40},
									  {"gold_block", 41},
									  {"iron_block", 42},
									  {"smooth_stone_slab", 43},
									  {"stone_slab", 44},
									  {"brick_wall", 45},
									  {"bricks", 45},
									  {"tnt", 46},
									  {"bookshelf", 47},
									  {"mossy_cobblestone", 48},
									  {"obsidian", 49},
									  {"torch", 50},
									  {"fire", 51},
									  {"spawner", 52},
									  {"oak_stairs", 53},
									  {"chest", 54},
									  {"redstone_wire", 55},
									  {"diamond_ore", 56},
									  {"diamond_block", 57},
									  {"crafting_table", 58},
									  {"wheat", 59},
									  {"farmland", 60},
									  {"furnace", 61},
									  {"campfire", 62},
									  {"oak_sign", 63},
									  {"oak_door", 64},
									  {"ladder", 65},
									  {"rail", 66},
									  {"stone_stairs", 67},
									  {"oak_wall_sign", 68},
									  {"lever", 69},
									  {"stone_pressure_plate", 70},
									  {"iron_door", 71},
									  {"oak_pressure_plate", 72},
									  {"redstone_ore", 73},
									  {"red_concrete", 74},
									  {"redstone_wall_torch", 75},
									  {"redstone_torch", 76},
									  {"stone_button", 77},
									  {"snow_block", 78},
									  {"ice", 79},
									  {"snow", 80},
									  {"cactus", 81},
									  {"clay", 82},
									  {"bamboo", 83},
									  {"jukebox", 84},
									  {"oak_fence", 85},
									  {"pumpkin", 86},
									  {"netherrack", 87},
									  {"soul_sand", 88},
									  {"glowstone", 89},
									  {"portal", 90},
									  {"carved_pumpkin", 91},
									  {"cake", 92},
									  {"repeater", 93},
									  {"skeleton_skull", 94},
									  {"white_stained_glass", 95},
									  {"oak_trapdoor", 96},
									  {"turtle_egg", 97},
									  {"stone_bricks", 98},
									  {"brown_mushroom_block", 99},
									  {"red_mushroom_block", 100},
									  {"iron_bars", 101},
									  {"light_blue_stained_glass_pane", 102},
									  {"melon", 103},
									  {"pumpkin_stem", 104},
									  {"melon_stem", 105},
									  {"vine", 106},
									  {"oak_fence_gate", 107},
									  {"brick_stairs", 108},
									  {"stone_brick_stairs", 109},
									  {"mycelium", 110},
									  {"light_gray_concrete", 111},
									  {"nether_brick", 112},
									  {"nether_brick_fence", 113},
									  {"nether_brick_stairs", 114},
									  {"nether_wart", 115},
									  {"enchanting_table", 116},
									  {"brewing_stand", 117},
									  {"cauldron", 118},
									  {"end_portal", 119},
									  {"end_portal_frame", 120},
									  {"end_stone", 121},
									  {"dragon_egg", 122},
									  {"redstone_lamp", 123},
									  {"shroomlight", 124},
									  {"oak_wood", 125},
									  {"oak_slab", 126},
									  {"cocoa", 127},
									  {"sandstone_stairs", 128},
									  {"emerald_ore", 129},
									  {"ender_chest", 130},
									  {"tripwire_hook", 131},
									  {"tripwire", 132},
									  {"emerald_block", 133},
									  {"spruce_stairs", 134},
									  {"birch_stairs", 135},
									  {"jungle_stairs", 136},
									  {"command_block", 137},
									  {"beacon", 138},
									  {"cobblestone_wall", 139},
									  {"flower_pot", 140},
									  {"carrots", 141},
									  {"potatoes", 142},
									  {"oak_button", 143},
									  {"skeleton_wall_skull", 144},
									  {"anvil", 145},
									  {"trapped_chest", 146},
									  {"light_weighted_pressure_plate", 147},
									  {"heavy_weighted_pressure_plate", 148},
									  {"comparator", 149},
									  {"chain", 150},
									  {"daylight_detector", 151},
									  {"redstone_block", 152},
									  {"nether_quartz_ore", 153},
									  {"hopper", 154},
									  {"quartz_block", 155},
									  {"quartz_stairs", 156},
									  {"activator_rail", 157},
									  {"dropper", 158},
									  {"pink_stained_glass", 159},
									  {"white_stained_glass_pane", 160},
									  {"dead_brain_coral", 161},
									  {"acacia_planks", 162},
									  {"acacia_stairs", 163},
									  {"dark_oak_stairs", 164},
									  {"slime_block", 165},
									  {"barrier", 166},
									  {"iron_trapdoor", 167},
									  {"prismarine", 168},
									  {"sea_lantern", 169},
									  {"hay_block", 170},
									  {"white_carpet", 171},
									  {"coarse_dirt", 172},
									  {"coal_block", 173},
									  {"packed_ice", 174},
									  {"orange_concrete", 175},
									  {"white_banner", 176},
									  {"white_wall_banner", 177},
									  {"white_concrete_powder", 178},
									  {"red_sandstone", 179},
									  {"red_sandstone_stairs", 180},
									  {"red_sandstone_wall", 181},
									  {"red_sandstone_slab", 182},
									  {"spruce_fence_gate", 183},
									  {"birch_fence_gate", 184},
									  {"jungle_fence_gate", 185},
									  {"dark_oak_fence_gate", 186},
									  {"acacia_fence_gate", 187},
									  {"spruce_fence", 188},
									  {"birch_fence", 189},
									  {"jungle_fence", 190},
									  {"dark_oak_fence", 191},
									  {"acacia_fence", 192},
									  {"spruce_door", 193},
									  {"birch_door", 194},
									  {"jungle_door", 195},
									  {"acacia_door", 196},
									  {"dark_oak_door", 197},
									  {"end_rod", 198},
									  {"chorus_plant", 199},
									  {"chorus_flower", 200},
									  {"purpur_block", 201},
									  {"purpur_pillar", 202},
									  {"purpur_stairs", 203},
									  {"purple_stained_glass", 204},
									  {"purpur_slab", 205},
									  {"end_stone_bricks", 206},
									  {"beetroots", 207},
									  {"grass_path", 208},
									  {"end_gateway", 209},
									  {"repeating_command_block", 210},
									  {"chain_command_block", 211},
									  {"frosted_ice", 212},
									  {"magma_block", 213},
									  {"nether_wart_block", 214},
									  {"red_nether_bricks", 215},
									  {"bone_block", 216},
									  {"structure_void", 217},
									  {"observer", 218},
									  {"white_shulker_box", 219},
									  {"orange_shulker_box", 220},
									  {"magenta_shulker_box", 221},
									  {"light_blue_shulker_box", 222},
									  {"yellow_shulker_box", 223},
									  {"lime_shulker_box", 224},
									  {"pink_shulker_box", 225},
									  {"gray_shulker_box", 226},
									  {"light_gray_shulker_box", 227},
									  {"cyan_shulker_box", 228},
									  {"purple_shulker_box", 229},
									  {"blue_shulker_box", 230},
									  {"brown_shulker_box", 231},
									  {"green_shulker_box", 232},
									  {"red_shulker_box", 233},
									  {"black_shulker_box", 234},
									  {"white_glazed_terracotta", 235},
									  {"orange_glazed_terracotta", 236},
									  {"magenta_glazed_terracotta", 237},
									  {"light_blue_glazed_terracotta", 238},
									  {"yellow_glazed_terracotta", 239},
									  {"lime_glazed_terracotta", 240},
									  {"pink_glazed_terracotta", 241},
									  {"gray_glazed_terracotta", 242},
									  {"light_gray_glazed_terracotta", 243},
									  {"cyan_glazed_terracotta", 244},
									  {"purple_glazed_terracotta", 245},
									  {"blue_glazed_terracotta", 246},
									  {"brown_glazed_terracotta", 247},
									  {"green_glazed_terracotta", 248},
									  {"red_glazed_terracotta", 249},
									  {"black_glazed_terracotta", 250},
									  {"gray_concrete", 251},
									  {"gray_concrete_powder", 252},
									  {"deepslate_redstone_ore", 1}, // TODO
									  {"deepslate_iron_ore", 1},	 // TODO
									  {"kelp", 1},					 // TODO
									  {"deepslate", 1},				 // TODO
									  {"moss_carpet", 1},			 // TODO
									  {"granite", 1},				 // TODO
									  {"diorite", 1},				 // TODO
									  {"glow_lichen", 1},			 // TODO
									  {"copper_ore", 1},			 // TODO
									  {"tuff", 1},					 // TODO
									  {"deepslate_gold_ore", 1},	 // TODO
									  {"flowering_azalea", 1},		 // TODO
									  {"azalea", 1},				 // TODO
									  {"cave_vines", 1},			 // TODO
									  {"cave_vines_plant", 1},		 // TODO
									  {"big_dripleaf", 1},			 // TODO
									  {"big_dripleaf_stem", 1},		 // TODO
									  {"spore_blossom", 1},			 // TODO
									  {"deepslate_diamond_ore", 1},	 // TODO
									  {"budding_amethyst", 1},		 // TODO
									  {"smooth_basalt", 1},			 // TODO
									  {"birch_log", 1},				 // TODO
									  {"cave_air", 1},				 // TODO
									  {"kelp_plant", 1},			 // TODO
									  {"deepslate_lapis_ore", 1},	 // TODO
									  {"deepslate_copper_ore", 1},	 // TODO
									  {"moss_block", 1},			 // TODO
									  {"small_amethyst_bud", 1},	 // TODO
									  {"calcite", 1},				 // TODO
									  {"amethyst_block", 1},		 // TODO
									  {"medium_amethyst_bud", 1},	 // TODO
									  {"large_amethyst_bud", 1},	 // TODO
									  {"amethyst_cluster", 1},		 // TODO
									  {"mossy_stone_bricks", 1},	 // TODO
									  {"cracked_stone_bricks", 1},	 // TODO
									  {"small_dripleaf", 1},		 // TODO
									  {"deepslate_coal_ore", 1},	 // TODO
									  {"fern", 1},					 // TODO
									  {"raw_copper_block", 1},		 // TODO
									  {"raw_iron_block", 1},		 // TODO
									  {"jungle_log", 1},			 // TODO
									  {"wall_torch", 1},			 // TODO
									  {"raw_iron_block", 1},		 // TODO
									  {"polished_granite", 1},		 // TODO
									  {"cut_sandstone", 1},			 // TODO
									  {"oxeye_daisy", 1},			 // TODO
									  {"sugar_cane", 1},			 // TODO
									  {"azure_bluet", 1},			 // TODO
									  {"cornflower", 1},			 // TODO
									  {"structure_block", 255}};
	return mcPalette;
}

bool MCRFormat::parsePaletteList(int dataVersion, const priv::NamedBinaryTag &palette, MinecraftSectionPalette &sectionPal) {
	// prior to 1.16 (2556) the palette entries used multiple 64bit fields
	// TODO const bool paletteMultiBits = dataVersion < 2556;
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
	const PaletteMap &map = getPaletteMap();
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
			// skip minecraft:
			const core::String key = value->substr(10);
			auto iter = map.find(key);
			if (iter == map.end()) {
				Log::debug("Could not find a color mapping for '%s'", key.c_str());
				sectionPal.pal[paletteEntry] = -1;
			} else {
				sectionPal.pal[paletteEntry] = iter->value;
			}
		}
		++paletteEntry;
	}
	return true;
}

#undef wrap

} // namespace voxel
