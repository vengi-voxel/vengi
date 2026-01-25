/**
 * @file
 */

#include "Nbt.h"
#include "Util.h"
#include "voxel/RawVolume.h"
#include <limits>

namespace voxelformat {
namespace nbt {

bool loadGroupsPalette(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
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
			const glm::ivec3 v = schematic::parsePosList(compound, "pos");
			mins = (glm::min)(mins, v);
			maxs = (glm::max)(maxs, v);
		}
		const voxel::Region region(mins, maxs);
		voxel::RawVolume *volume = new voxel::RawVolume(region);
		for (const priv::NamedBinaryTag &compound : list) {
			const int state = compound.get("state").int32();
			const glm::ivec3 v = schematic::parsePosList(compound, "pos");
			volume->setVoxel(v, voxel::createVoxel(palette, state));
		}
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		const priv::NamedBinaryTag &author = schematic.get("author");
		if (author.valid() && author.type() == priv::TagType::STRING) {
			node.setProperty(scenegraph::PropAuthor, author.string());
		}
		node.setVolume(volume, true);
		node.setPalette(palette);
		int nodeId = sceneGraph.emplace(core::move(node));
		return nodeId != InvalidNodeId;
	}
	Log::error("Could not find valid 'blocks' tags");
	return false;
}

} // namespace nbt
} // namespace voxelformat
