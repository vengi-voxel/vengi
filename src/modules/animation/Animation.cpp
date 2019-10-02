/**
 * @file
 */

#include "Animation.h"
#include "core/Common.h"
#include "core/Array.h"
#include <string.h>

namespace animation {

const char* toString(Animation anim) {
	static const char *_strings[] = { "idle", "jump", "run", "glide", "tool" };
	static_assert(lengthof(_strings) == std::enum_value(Animation::Max), "Invalid animation array dimensions");
	return _strings[std::enum_value(anim)];
}

static const char *_ToolAnimationTypeString[] = { "none", "swing", "stroke", "tense", "twiddle" };
static_assert(lengthof(_ToolAnimationTypeString) == std::enum_value(ToolAnimationType::Max), "Invalid tool animation array dimensions");

const char* toString(ToolAnimationType anim) {
	return _ToolAnimationTypeString[std::enum_value(anim)];
}

ToolAnimationType toToolAnimationEnum(const char* anim) {
	if (anim == nullptr) {
		return ToolAnimationType::None;
	}
	for (int i = 0; i < lengthof(_ToolAnimationTypeString); ++i) {
		if (!strcmp(anim, _ToolAnimationTypeString[i])) {
			return (ToolAnimationType)i;
		}
	}
	return ToolAnimationType::Max;
}

}
