/**
 * @file
 */

#include "Animation.h"
#include "core/Common.h"
#include "core/ArrayLength.h"
#include "core/Enum.h"
#include <SDL_stdinc.h>

namespace animation {

const char* toString(Animation anim) {
	return network::EnumNameAnimation(anim);
}

static const char *_ToolAnimationTypeString[] = { "none", "swing", "stroke", "tense", "twiddle" };
static_assert(lengthof(_ToolAnimationTypeString) == core::enumVal(ToolAnimationType::Max), "Invalid tool animation array dimensions");

const char* toString(ToolAnimationType anim) {
	return _ToolAnimationTypeString[core::enumVal(anim)];
}

ToolAnimationType toToolAnimationEnum(const char* anim) {
	if (anim == nullptr) {
		return ToolAnimationType::None;
	}
	for (int i = 0; i < lengthof(_ToolAnimationTypeString); ++i) {
		if (!SDL_strcmp(anim, _ToolAnimationTypeString[i])) {
			return (ToolAnimationType)i;
		}
	}
	return ToolAnimationType::Max;
}

}
