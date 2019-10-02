/**
 * @file
 */

#include "CharacterMeshType.h"
#include "core/Array.h"
#include "core/Common.h"

namespace animation {

const char *toString(CharacterMeshType type) {
	static const char *_strings[] = { "head", "chest", "belt", "pants", "hand", "foot", "shoulder", "glider" };
	static_assert(lengthof(_strings) == std::enum_value(CharacterMeshType::Max), "Invalid animation array dimensions");
	return _strings[std::enum_value(type)];
}

}
