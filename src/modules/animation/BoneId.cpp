/**
 * @file
 */

#include "BoneId.h"
#include "core/ArrayLength.h"
#include "core/Common.h"
#include "core/Enum.h"
#include <SDL_stdinc.h>

namespace animation {

static const char *boneId_strings[] = { "head", "chest", "belt", "pants",
		"lefthand", "righthand", "leftfoot", "rightfoot", "tool",
		"leftshoulder", "rightshoulder", "glider", "torso",
		"leftwing", "rightwing", "tail", "body" };
static_assert(lengthof(boneId_strings) == core::enumVal(BoneId::Max), "Invalid bone array dimensions");

BoneId toBoneId(const char *name) {
	for (int i = 0; i < lengthof(boneId_strings); ++i) {
		if (!SDL_strcmp(name, boneId_strings[i])) {
			return (BoneId)i;
		}
	}
	return BoneId::Max;
}

const char* toBoneId(const BoneId id) {
	return boneId_strings[core::enumVal(id)];
}

}
