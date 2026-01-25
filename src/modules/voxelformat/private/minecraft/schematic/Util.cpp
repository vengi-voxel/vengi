/**
 * @file
 */

#include "Util.h"
#include "core/Log.h"
#include <glm/vec3.hpp>

namespace voxelformat {
namespace schematic {

glm::ivec3 parsePosList(const priv::NamedBinaryTag &compound, const core::String &key) {
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

} // namespace schematic
} // namespace voxelformat
