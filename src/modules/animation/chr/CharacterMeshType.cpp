/**
 * @file
 */

#include "CharacterMeshType.h"
#include "core/Array.h"
#include "core/Common.h"

namespace animation {

static const char *_strings[] = { "head", "chest", "belt", "pants", "hand", "foot", "shoulder", "glider" };
static_assert(lengthof(_strings) == std::enum_value(CharacterMeshType::Max), "Invalid animation array dimensions");

const char *toString(CharacterMeshType type) {
	return _strings[std::enum_value(type)];
}

CharacterMeshType toEnum(const char *type) {
	for (int i = 0; i < lengthof(_strings); ++i) {
		if (!strcmp(type, _strings[i])) {
			return (CharacterMeshType)i;
		}
	}
	return CharacterMeshType::Max;
}


}
