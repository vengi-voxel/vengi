/**
 * @file
 */

#include "SchematicFormat.h"
#include "MinecraftPaletteMap.h"
#include "NamedBinaryTag.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "schematic/Axiom.h"
#include "schematic/Litematic.h"
#include "schematic/Nbt.h"
#include "schematic/Sponge.h"

#include <glm/common.hpp>

namespace voxelformat {

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
		return axiom::loadGroupsPalette(*stream, sceneGraph, palette);
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
		if (nbt::loadGroupsPalette(schematic, sceneGraph, palette, dataVersion)) {
			return true;
		}
	} else if (extension == "litematic") {
		return litematic::loadGroupsPalette(schematic, sceneGraph, palette);
	}

	const int version = schematic.get("Version").int32(-1);
	Log::debug("Load schematic version %i", version);
	if (version >= 3) {
		if (sponge::loadGroupsPaletteSponge3(schematic, sceneGraph, palette, version)) {
			return true;
		}
	}
	return sponge::loadGroupsPaletteSponge1And2(schematic, sceneGraph, palette);
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
		return axiom::loadScreenshot(stream);
	}
	return {};
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

	const core::VarPtr &schematicType = core::getVar(cfg::VoxformatSchematicType);
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

					Log::debug("Set block state %s at %i %i %i to %i", blockState.c_str(), x, y, z, (int)blockDataIdx);
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
