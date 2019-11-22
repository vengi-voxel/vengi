/**
 * @file
 */

#include "Skeleton.h"
#include "core/Common.h"
#include "animation/BoneUtil.h"

namespace animation {

void Skeleton::lerp(const Skeleton& previous, float dt) {
	const float factor = glm::min(1.0f, dt);
	for (int i = 0; i < std::enum_value(BoneId::Max); ++i) {
		const BoneId id = (BoneId)i;
		bone(id).lerp(previous.bone(id), factor);
	}
}

}
