/**
 * @file
 */

#include "Animation.h"
#include "core/Common.h"
#include "core/ArrayLength.h"
#include <string.h>

namespace animation {

const char* toString(Animation anim) {
	return network::EnumNameAnimation(anim);
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
