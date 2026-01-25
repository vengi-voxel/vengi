/**
 * @file
 */

#include "Litematic.h"
#include "../MinecraftPaletteMap.h"
#include "Util.h"
#include "voxel/RawVolume.h"

namespace voxelformat {
namespace litematic {

static bool readLitematicBlockStates(const glm::ivec3 &size, int nbtPaletteSize,
									 const priv::NamedBinaryTag &blockStates, scenegraph::SceneGraphNode &node,
									 const schematic::SchematicPalette &mcpal) {
	const core::Buffer<int64_t> *data = blockStates.longArray();
	if (data == nullptr) {
		Log::error("Invalid BlockStates - expected long array");
		return false;
	}

	int bits = 0;
	while (nbtPaletteSize > (1 << bits)) {
		++bits;
	}
	bits = core_max(bits, 2);

	voxel::RawVolume *v = node.volume();
	const palette::Palette &palette = node.palette();
	core::AtomicBool success{true};
	app::for_parallel(0, size.y, [bits, size, data, &mcpal, v, &palette, &success](int start, int end) {
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
						id = (((uint64_t)(*data)[startIdx]) >> rshiftVal | ((uint64_t)(*data)[startIdx + 1])
																			   << move_num_2) &
							 mask;
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

bool loadGroupsPalette(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
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
			const glm::ivec3 &pos = schematic::parsePosList(regionCompound, "Position");
			const glm::ivec3 &size = glm::abs(schematic::parsePosList(regionCompound, "Size"));
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
			schematic::SchematicPalette mcpal;
			mcpal.resize(blockStatePaletteNbt.size());
			int paletteSize = 0;
			for (const auto &palNbt : blockStatePaletteNbt) {
				const priv::NamedBinaryTag &materialName = palNbt.get("Name");
				mcpal[paletteSize++] = findPaletteIndex(materialName.string()->c_str(), 1);
			}
			const int n = (int)blockStatePaletteNbt.size();

			const priv::NamedBinaryTag &blockStates = regionCompound.get("BlockStates");
			if (!blockStates.valid() || blockStates.type() != priv::TagType::LONG_ARRAY) {
				Log::error("Could not find 'BlockStates'");
				return false;
			}
			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
			node.setPalette(palette);
			node.setName(name);
			node.setVolume(new voxel::RawVolume(region), true);
			if (!readLitematicBlockStates(size, n, blockStates, node, mcpal)) {
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

} // namespace litematic
} // namespace voxelformat
