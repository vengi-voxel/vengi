/**
 * @file
 */

#include "BoneId.h"
#include "core/Array.h"
#include "core/Common.h"

namespace animation {

static const char *boneId_strings[] = { "head", "chest", "belt", "pants",
		"lefthand", "righthand", "leftfoot", "rightfoot", "tool",
		"leftshoulder", "rightshoulder", "glider", "torso",
		"leftwing", "rightwing", "tail" };
static_assert(lengthof(boneId_strings) == std::enum_value(BoneId::Max), "Invalid bone array dimensions");

BoneId toBoneId(const char *name) {
	for (int i = 0; i < lengthof(boneId_strings); ++i) {
		if (!strcmp(name, boneId_strings[i])) {
			return (BoneId)i;
		}
	}
	return BoneId::Max;
}

const char* toBoneId(const BoneId id) {
	return boneId_strings[std::enum_value(id)];
}

}
