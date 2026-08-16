/**
 * @file
 */

#include "MCRFormat.h"
#include "app/Async.h"
#include "color/Color.h"
#include "color/RGBA.h"
#include "core/Common.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/concurrent/Atomic.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include "palette/Material.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraph.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeMerger.h"
#include "voxelutil/VolumeVisitor.h"
#include "MinecraftPaletteMap.h"
#include "NamedBinaryTag.h"

#include <glm/common.hpp>

namespace voxelformat {

namespace {

core::String blockStateFromCompound(const priv::NamedBinaryTag &block) {
	const core::String *name = block.get("Name").string();
	if (name == nullptr) {
		return {};
	}
	core::String state = *name;
	const priv::NamedBinaryTag &props = block.get("Properties");
	if (props.valid() && props.type() == priv::TagType::COMPOUND) {
		for (const auto &entry : *props.compound()) {
			const core::String *val = entry->value.string();
			if (val == nullptr) {
				continue;
			}
			state.append(",").append(entry->key).append("=").append(*val);
		}
	}
	return state;
}

} // namespace

constexpr float MinecraftWaterAlpha = 0.5f;

voxel::RawVolume *MCRFormat::extractWaterVolume(voxel::RawVolume *terrain) {
	if (terrain == nullptr) {
		return nullptr;
	}
	const voxel::Region region = terrain->region();
	voxel::RawVolume *water = new voxel::RawVolume(region);
	core::AtomicInt hasWater{0};
	app::for_parallel(region.getLowerY(), region.getUpperY() + 1, [terrain, water, &region, &hasWater](int start, int end) {
		voxel::RawVolume::Sampler terrainSampler(terrain);
		voxel::RawVolume::Sampler waterSampler(water);
		terrainSampler.setPosition(region.getLowerX(), start, region.getLowerZ());
		waterSampler.setPosition(region.getLowerX(), start, region.getLowerZ());
		for (int y = start; y < end; ++y) {
			voxel::RawVolume::Sampler terrain2 = terrainSampler;
			voxel::RawVolume::Sampler water2 = waterSampler;
			for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
				voxel::RawVolume::Sampler terrain3 = terrain2;
				voxel::RawVolume::Sampler water3 = water2;
				for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
					const voxel::Voxel voxel = terrain3.voxel();
					if (voxel::isTransparent(voxel.getMaterial())) {
						water3.setVoxel(voxel);
						terrain3.setVoxel(voxel::Voxel());
						hasWater.increment();
					}
					terrain3.movePositiveX();
					water3.movePositiveX();
				}
				terrain2.movePositiveZ();
				water2.movePositiveZ();
			}
			terrainSampler.movePositiveY();
			waterSampler.movePositiveY();
		}
	});
	if ((int)hasWater == 0) {
		delete water;
		return nullptr;
	}
	if (voxel::RawVolume *cropped = voxelutil::cropVolume(water)) {
		delete water;
		return cropped;
	}
	return water;
}

void MCRFormat::applyWaterTransparency(palette::Palette &palette, const voxel::RawVolume *water) {
	if (water == nullptr) {
		return;
	}
	uint8_t used[256]{};
	voxelutil::visitVolume(
		*water,
		[&used](int, int, int, const voxel::Voxel &voxel) {
			used[voxel.getColor()] = 1;
		},
		voxelutil::VisitSolid());
	const int colorCount = palette.colorCount();
	for (int i = 0; i < colorCount; ++i) {
		if (used[i] == 0) {
			continue;
		}
		color::RGBA c = palette.color((uint8_t)i);
		c.a = (uint8_t)(MinecraftWaterAlpha * 255.0f + 0.5f);
		palette.setColor((uint8_t)i, c);
		palette.setMaterialType((uint8_t)i, palette::MaterialType::Glass);
	}
}

scenegraph::SceneGraphNode MCRFormat::createWaterNode(voxel::RawVolume *water, const palette::Palette &palette) {
	scenegraph::SceneGraphNode waterNode(scenegraph::SceneGraphNodeType::Model);
	waterNode.setName(WaterNodeName);
	waterNode.setProperty("mctype", "water");
	waterNode.setVolume(water);
	palette::Palette waterPalette = palette;
	applyWaterTransparency(waterPalette, water);
	waterNode.setPalette(waterPalette);
	return waterNode;
}

#define wrap(expression)                                                                                               \
	do {                                                                                                               \
		if ((expression) != 0) {                                                                                       \
			Log::error("Could not load file: Not enough data in stream " CORE_STRINGIFY(#expression) " at " CORE_FILE  \
																									 ":%i",            \
					   CORE_LINE);                                                                                     \
			return false;                                                                                              \
		}                                                                                                              \
	} while (0)

#define wrapNull(expression)                                                                                               \
	do {                                                                                                               \
		if ((expression) != 0) {                                                                                       \
			Log::error("Could not load file: Not enough data in stream " CORE_STRINGIFY(#expression) " at " CORE_FILE  \
																									 ":%i",            \
					   CORE_LINE);                                                                                     \
			return nullptr;                                                                                              \
		}                                                                                                              \
	} while (0)

bool MCRFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
								  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream2(archive->readStream(filename));
	if (!stream2) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	io::BufferedReadWriteStream bufferedStream(*stream2, stream2->size());
	bufferedStream.seek(0);
	const int64_t length = bufferedStream.size();
	if (length < SECTOR_BYTES) {
		Log::debug("File does not contain enough data: %s", filename.c_str());
		return false;
	}

	core::String name = core::string::extractFilenameWithExtension(filename.toLower());
	int chunkX = 0;
	int chunkZ = 0;
	char type = 'a';
	if (SDL_sscanf(name.c_str(), "r.%i.%i.mc%c", &chunkX, &chunkZ, &type) != 3) {
		Log::warn("Failed to parse the region chunk boundaries from filename %s (%i.%i.%c)", name.c_str(), chunkX,
				  chunkZ, type);
	}

	palette.minecraft();
	switch (type) {
	case 'r':	// Region file format
	case 'a': { // Anvil file format
		const int64_t fileSize = bufferedStream.remaining();
		if (fileSize < 2l * SECTOR_BYTES) {
			Log::error("This region file has not enough data for the 8kb header");
			return false;
		}

		Offsets offsets;
		for (int i = 0; i < SECTOR_INTS; ++i) {
			uint8_t raw[3];
			wrap(bufferedStream.readUInt8(raw[0]));
			wrap(bufferedStream.readUInt8(raw[1]));
			wrap(bufferedStream.readUInt8(raw[2]));
			wrap(bufferedStream.readUInt8(offsets[i].sectorCount));

			offsets[i].offset = ((raw[0] << 16) + (raw[1] << 8) + raw[2]) * SECTOR_BYTES;
		}

		for (int i = 0; i < SECTOR_INTS; ++i) {
			uint32_t lastModValue;
			wrap(bufferedStream.readUInt32BE(lastModValue));
		}

		// might be an empty region file
		if (bufferedStream.eos()) {
			Log::debug("Empty region file: %s", filename.c_str());
			return false;
		}

		voxel::RawVolume *volumes[SECTOR_INTS]{};
		core::AtomicInt sectorsDone{0};
		auto fn = [&volumes, &offsets, palette, &bufferedStream, &ctx, &sectorsDone, this](int start, int end) {
			io::MemoryReadStream memStream(bufferedStream.getBuffer(), bufferedStream.size());
			Log::debug("Loading sectors from %i to %i", start, end);
			for (int i = start; i < end; ++i) {
				if (offsets[i].sectorCount == 0u || offsets[i].offset < sizeof(offsets)) {
					const int completed = sectorsDone.increment() + 1;
					ctx.report("chunk", completed, SECTOR_INTS);
					continue;
				}
				if (offsets[i].offset + 6 >= (uint32_t)memStream.size()) {
					const int completed = sectorsDone.increment() + 1;
					ctx.report("chunk", completed, SECTOR_INTS);
					continue;
				}
				if (memStream.seek(offsets[i].offset) == -1) {
					const int completed = sectorsDone.increment() + 1;
					ctx.report("chunk", completed, SECTOR_INTS);
					continue;
				}
				volumes[i] = readCompressedNBT(memStream, i, palette);
				const int completed = sectorsDone.increment() + 1;
				ctx.report("chunk", completed, SECTOR_INTS);
			}
		};
		app::for_parallel(0, SECTOR_INTS, fn);

		int added = 0;
		const bool separateWater = core::getVar(cfg::VoxformatMCSeparateWater)->boolVal();
		for (int i = 0; i < SECTOR_INTS; ++i) {
			if (volumes[i] == nullptr) {
				continue;
			}
			voxel::RawVolume *water = nullptr;
			if (separateWater) {
				water = extractWaterVolume(volumes[i]);
				if (water != nullptr) {
					if (voxel::RawVolume *cropped = voxelutil::cropVolume(volumes[i])) {
						delete volumes[i];
						volumes[i] = cropped;
					} else if (volumes[i]->isEmpty(volumes[i]->region())) {
						delete volumes[i];
						volumes[i] = nullptr;
					}
				}
			}
			int parentId = 0;
			if (volumes[i] != nullptr) {
				scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
				node.setVolume(volumes[i]);
				node.setPalette(palette);
				parentId = sceneGraph.emplace(core::move(node));
				if (parentId == InvalidNodeId) {
					parentId = 0;
				} else {
					++added;
				}
			}
			if (water != nullptr) {
				const int waterId = sceneGraph.emplace(createWaterNode(water, palette), parentId);
				if (waterId != InvalidNodeId) {
					++added;
				}
			}
		}
		ctx.report("chunk", SECTOR_INTS, SECTOR_INTS);

		return added > 0;
	}
	}
	Log::error("Unkown file type given: %c", type);
	return false;
}

voxel::RawVolume *MCRFormat::readCompressedNBT(io::SeekableReadStream &stream, int sector,
											   const palette::Palette &palette) const {
	uint32_t nbtSize;
	wrapNull(stream.readUInt32BE(nbtSize));
	if (nbtSize == 0) {
		Log::debug("Empty nbt chunk found");
		return nullptr;
	}

	if (nbtSize > 0x1FFFFFF) {
		Log::error("Size of nbt data exceeds the max allowed value: %u", nbtSize);
		return nullptr;
	}

	uint8_t version;
	wrapNull(stream.readUInt8(version));
	if (version != VERSION_GZIP && version != VERSION_DEFLATE) {
		Log::error("Unsupported version found: %u", version);
		return nullptr;
	}

	// the version is included in the length
	--nbtSize;

	io::ZipReadStream zipStream(stream, (int)nbtSize);
	priv::NamedBinaryTagContext ctx;
	ctx.stream = &zipStream;
	const priv::NamedBinaryTag &root = priv::NamedBinaryTag::parse(ctx);
	if (!root.valid()) {
		Log::error("Could not parse nbt structure");
		return nullptr;
	}

	// https://minecraft.wiki/w/Data_version
	const int32_t dataVersion = root.get("DataVersion").int32();
	Log::debug("Found data version %i", dataVersion);
	if (dataVersion >= 2844) {
		return parseSections(dataVersion, root, sector, palette);
	}
	return parseLevelCompound(dataVersion, root, sector, palette);
}

int MCRFormat::getVoxel(int dataVersion, const priv::NamedBinaryTag &data, int x, int y, int z) {
	const uint32_t i = y * MAX_SIZE * MAX_SIZE + z * MAX_SIZE + x;
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

voxel::RawVolume *MCRFormat::error(SectionVolumes &volumes) const {
	for (voxel::RawVolume *v : volumes) {
		delete v;
	}
	return nullptr;
}

voxel::RawVolume *MCRFormat::finalize(SectionVolumes &volumes, int xPos, int zPos) const {
	if (volumes.empty()) {
		Log::debug("No volumes found at %i:%i", xPos, zPos);
		return nullptr;
	}
	voxel::RawVolume *merged = voxelutil::merge(volumes);
	for (voxel::RawVolume *v : volumes) {
		delete v;
	}
	merged->translate(glm::ivec3(xPos * MAX_SIZE, 0, zPos * MAX_SIZE));
	if (voxel::RawVolume *cropped = voxelutil::cropVolume(merged)) {
		delete merged;
		return cropped;
	}
	return merged;
}

bool MCRFormat::parseBlockStates(int dataVersion, const palette::Palette &palette, const priv::NamedBinaryTag &data,
								 SectionVolumes &volumes, int sectionY, const MinecraftSectionPalette &secPal) const {
	Log::debug("Parse block states");
	const bool hasData = data.type() == priv::TagType::LONG_ARRAY && !data.longArray()->empty();

	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(MAX_SIZE - 1, MAX_SIZE - 1, MAX_SIZE - 1);
	const voxel::Region region(mins, maxs);
	voxel::RawVolume *v = new voxel::RawVolume(region);
	bool hasBlocks = false;
	// findPaletteIndex / legacy block ids already index the minecraft palette that MCR loads into
	// `palette`. Skip PaletteLookup (1M uint16_t memset) unless a remap palette was provided on secPal.
	const bool samePalette = secPal.mcpal.colorCount() == 0 || palette.hash() == secPal.mcpal.hash();
	const bool separateWater = core::getVar(cfg::VoxformatMCSeparateWater)->boolVal();
	auto createBlockVoxel = [&palette, separateWater](uint8_t palColIdx, bool water) {
		if (separateWater && water) {
			return voxel::createVoxel(voxel::VoxelType::Transparent, palColIdx);
		}
		return voxel::createVoxel(palette, palColIdx);
	};

	if (secPal.pal.empty()) {
		if (data.type() != priv::TagType::BYTE_ARRAY) {
			Log::error("Unknown block data type: %i for version %i", (int)data.type(), dataVersion);
			delete v;
			return false;
		}
		bool error = false;
		core::ScopedPtr<palette::PaletteLookup> palLookup;
		if (!samePalette) {
			palLookup = new palette::PaletteLookup(palette);
		}
		auto fn = [&] (int start, int end) {
			voxel::RawVolume::Sampler sampler(v);
			sampler.setPosition(0, start, 0);
			for (int y = start; y < end; ++y) {
				voxel::RawVolume::Sampler sampler2 = sampler;
				for (int z = 0; z < MAX_SIZE; ++z) {
					voxel::RawVolume::Sampler sampler3 = sampler2;
					for (int x = 0; x < MAX_SIZE; ++x) {
						const int color = getVoxel(dataVersion, data, x, y, z);
						if (color < 0) {
							Log::error("Failed to load voxel at position %i:%i:%i (dataversion: %i)", x, y,
									z, dataVersion);
							delete v;
							error = true;
							return;
						}
						if (color) {
							const uint8_t palColIdx =
								samePalette ? (uint8_t)color
											: palLookup->findClosestIndex(secPal.mcpal.color(color));
							const voxel::Voxel voxel = createBlockVoxel(palColIdx, isLegacyWaterId(color));
							sampler3.setVoxel(voxel);
							hasBlocks = true;
						}
						sampler3.movePositiveX();
					}
					sampler2.movePositiveZ();
				}
				sampler.movePositiveY();
			}
		};
		app::for_parallel(0, MAX_SIZE, fn);
		if (error) {
			return false;
		}
	} else if (hasData) {
		if (data.type() != priv::TagType::LONG_ARRAY) {
			Log::error("Unknown block data type: %i for version %i", (int)data.type(), dataVersion);
			delete v;
			return false;
		}

		const core::Buffer<int64_t> &blockStates = *data.longArray();

		constexpr int blockCount = MAX_SIZE * MAX_SIZE * MAX_SIZE;
		uint8_t blocks[blockCount];
		uint8_t waterFlags[blockCount]{};
		int bsCnt = 0;
		size_t bitCnt = 0;
		if (dataVersion < 2529) {
			const size_t bitSize = (data.longArray()->size()) * 64 / blockCount;
			const uint32_t bitMask = (1 << bitSize) - 1;
			for (int i = 0; i < blockCount; i++) {
				if (bitCnt + bitSize <= 64) {
					const uint64_t blockState = blockStates[bsCnt];
					const uint64_t blockIndex = (blockState >> bitCnt) & bitMask;
					if (blockIndex < secPal.pal.size()) {
						blocks[i] = secPal.pal[blockIndex];
						if (blockIndex < secPal.isWater.size()) {
							waterFlags[i] = secPal.isWater[blockIndex];
						}
						hasBlocks = true;
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
						if (blockIndex < secPal.isWater.size()) {
							waterFlags[i] = secPal.isWater[blockIndex];
						}
						hasBlocks = true;
					} else {
						blocks[i] = 0;
					}
				}
			}
		} else {
				const size_t bitSize = secPal.numBits;
				const uint32_t bitMask = bitSize > 0 ? ((1u << bitSize) - 1u) : 0u;
				for (int i = 0; i < blockCount; ++i) {
					uint32_t blockIndex = 0;
					if (bitSize > 0) {
						if (bitCnt + bitSize > 64) {
							bsCnt++;
							bitCnt = 0;
						}
						const uint64_t blockState = blockStates[bsCnt];
						blockIndex = (uint32_t)((blockState >> bitCnt) & bitMask);
						bitCnt += bitSize;
					}
					if (blockIndex < secPal.pal.size()) {
						blocks[i] = secPal.pal[blockIndex];
						if (blockIndex < secPal.isWater.size()) {
							waterFlags[i] = secPal.isWater[blockIndex];
						}
						if (blocks[i] != 0) {
							hasBlocks = true;
						}
					} else {
						blocks[i] = 0;
					}
				}
		}

		core::ScopedPtr<palette::PaletteLookup> palLookup;
		if (!samePalette) {
			palLookup = new palette::PaletteLookup(palette);
		}
		auto fn = [&] (int start, int end) {
			voxel::RawVolume::Sampler sampler(v);
			sampler.setPosition(0, start, 0);
			for (int y = start; y < end; ++y) {
				voxel::RawVolume::Sampler sampler2 = sampler;
				for (int z = 0; z < MAX_SIZE; ++z) {
					voxel::RawVolume::Sampler sampler3 = sampler2;
					for (int x = 0; x < MAX_SIZE; ++x) {
						const uint16_t i = y * MAX_SIZE * MAX_SIZE + z * MAX_SIZE + x;
						const uint8_t color = blocks[i];
						if (color) {
							const uint8_t palColIdx =
								samePalette ? color
											: palLookup->findClosestIndex(secPal.mcpal.color(color));
							const voxel::Voxel voxel = createBlockVoxel(palColIdx, waterFlags[i] != 0);
							sampler3.setVoxel(voxel);
						}
						sampler3.movePositiveX();
					}
					sampler2.movePositiveZ();
				}
				sampler.movePositiveY();
			}
		};
		app::for_parallel(0, MAX_SIZE, fn);
	} else if (!secPal.pal.empty()) {
		const uint8_t color = secPal.pal[0];
		if (color != 0) {
			core::ScopedPtr<palette::PaletteLookup> palLookup;
			if (!samePalette) {
				palLookup = new palette::PaletteLookup(palette);
			}
			const uint8_t palColIdx =
				samePalette ? color : palLookup->findClosestIndex(secPal.mcpal.color(color));
			const bool water = !secPal.isWater.empty() && secPal.isWater[0] != 0;
			const voxel::Voxel voxel = createBlockVoxel(palColIdx, water);
			auto fn = [&] (int start, int end) {
				voxel::RawVolume::Sampler sampler(v);
				sampler.setPosition(0, start, 0);
				for (int y = start; y < end; ++y) {
					voxel::RawVolume::Sampler sampler2 = sampler;
					for (int z = 0; z < MAX_SIZE; ++z) {
						voxel::RawVolume::Sampler sampler3 = sampler2;
						for (int x = 0; x < MAX_SIZE; ++x) {
							sampler3.setVoxel(voxel);
							sampler3.movePositiveX();
						}
						sampler2.movePositiveZ();
					}
					sampler.movePositiveY();
				}
			};
			app::for_parallel(0, MAX_SIZE, fn);
			hasBlocks = true;
		}
	}

	if (hasBlocks) {
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

voxel::RawVolume *MCRFormat::parseSections(int dataVersion, const priv::NamedBinaryTag &root, int sector,
										   const palette::Palette &pal) const {
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
			Log::debug("Could not find 'block_states'");
			continue;
		}
		const priv::NamedBinaryTag &ylvl = section.get("Y");
		if (!ylvl.valid()) {
			Log::debug("Could not find Y int in section compound");
		}
		const int8_t sectionY = ylvl.int8();
		if (sectionY == -1) {
			Log::debug("Skip empty section compound");
		}
		Log::debug("Y level for section compound: %i", (int)sectionY);

		const priv::NamedBinaryTag &paletteTag = blockStates.get("palette");
		MinecraftSectionPalette secPal;
		if (paletteTag.valid()) {
			if (!parsePaletteList(dataVersion, paletteTag, secPal)) {
				Log::error("Could not parse palette chunk");
				return error(volumes);
			}
		} else {
			secPal.pal.resize(1);
			secPal.pal[0] = (uint8_t)findPaletteIndex("minecraft:air", 0);
			secPal.numBits = 0u;
		}
		const priv::NamedBinaryTag &data = blockStates.get("data");
		if (!parseBlockStates(dataVersion, pal, data, volumes, sectionY, secPal)) {
			Log::error("Failed to parse 'data' tag");
			return error(volumes);
		}
	}
	return finalize(volumes, xPos, zPos);
}

voxel::RawVolume *MCRFormat::parseLevelCompound(int dataVersion, const priv::NamedBinaryTag &root, int sector,
												const palette::Palette &pal) const {
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
			Log::debug("Status for level node is not postprocessed but %s (version: %i)", tagStatus->c_str(),
					   dataVersion);
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
		return nullptr;
	}
	SectionVolumes volumes;
	for (const priv::NamedBinaryTag &section : sectionsList) {
		const priv::NamedBinaryTag &ylvl = section.get("Y");
		if (!ylvl.valid()) {
			Log::debug("Could not find Y int in section compound");
		}
		const int8_t sectionY = ylvl.int8();
		if (sectionY == -1) {
			Log::debug("Skip empty section compound");
		}
		Log::debug("Y level for section compound: %i", (int)sectionY);

		MinecraftSectionPalette secPal;

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
		// const priv::NamedBinaryTag &data = section.get("Data");
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

bool MCRFormat::parsePaletteList(int dataVersion, const priv::NamedBinaryTag &palette,
								 MinecraftSectionPalette &sectionPal) const {
	if (palette.type() != priv::TagType::LIST) {
		Log::error("Invalid type for palette: %i", (int)palette.type());
		return false;
	}
	const priv::NBTList &paletteList = *palette.list();
	const size_t paletteCount = paletteList.size();
	if (paletteCount > 4096u) {
		Log::error("Palette overflow");
		return false;
	}
	sectionPal.pal.resize(paletteCount);
	sectionPal.isWater.resize(paletteCount);
	if (paletteCount <= 1u) {
		sectionPal.numBits = 0u;
	} else {
		sectionPal.numBits = (uint32_t)glm::max(glm::ceil(glm::log2((float)paletteCount)), 4.0f);
	}

	int paletteEntry = 0;
	for (const priv::NamedBinaryTag &block : paletteList) {
		if (block.type() != priv::TagType::COMPOUND) {
			Log::error("Invalid block type %i", (int)block.type());
			return false;
		}

		const core::String state = blockStateFromCompound(block);
		if (state.empty()) {
			Log::error("Could not parse block state from palette entry %i", paletteEntry);
			return false;
		}
		sectionPal.pal[paletteEntry] = (uint8_t)findPaletteIndex(state, 1);
		sectionPal.isWater[paletteEntry] = isWaterBlock(state) ? 1 : 0;
		++paletteEntry;
	}
	return true;
}

#undef wrap
#undef wrapNull

#define wrapBool(write)                                                                                                \
	if ((write) == false) {                                                                                            \
		Log::error("Could not save mcr file: " CORE_STRINGIFY(write));                                                 \
		return false;                                                                                                  \
	}

bool MCRFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}

	Offsets offsets;

	for (int i = 0; i < SECTOR_INTS; ++i) {
		uint8_t raw[3] = {0, 0, 0};	 // TODO
		offsets[i].sectorCount = 0; // TODO

		core_assert(offsets[i].offset < sizeof(offsets));
		wrapBool(stream->writeUInt8(raw[0]));
		wrapBool(stream->writeUInt8(raw[1]));
		wrapBool(stream->writeUInt8(raw[2]));
		wrapBool(stream->writeUInt8(offsets[i].sectorCount));
	}

	for (int i = 0; i < SECTOR_INTS; ++i) {
		uint32_t lastModValue = 0u;
		wrapBool(stream->writeUInt32BE(lastModValue));
	}

	return saveMinecraftRegion(sceneGraph, *stream, offsets);
}

bool MCRFormat::saveMinecraftRegion(const scenegraph::SceneGraph &sceneGraph, io::SeekableWriteStream &stream, const Offsets &offsets) {
	for (int i = 0; i < SECTOR_INTS; ++i) {
		if (offsets[i].sectorCount == 0u) {
			continue;
		}
		if (!saveCompressedNBT(sceneGraph, stream, i)) {
			Log::error("Failed to save minecraft chunk section %i for offset %u", i, (int)offsets[i].offset);
			return false;
		}
	}
	return true;
}

bool MCRFormat::saveCompressedNBT(const scenegraph::SceneGraph &sceneGraph, io::SeekableWriteStream &stream,
								  int sector) {
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
	wrapBool(stream.writeUInt32BE((uint32_t)nbtSize))
	return stream.seek(nbtEndOffset) != -1;
}

bool MCRFormat::saveSections(const scenegraph::SceneGraph &sceneGraph, priv::NBTList &sections, int sector) {
#if 0
	for (const scenegraph::SceneGraphNode &node : sceneGraph) {
		// 16x256x16 chunks needs to be saved
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
