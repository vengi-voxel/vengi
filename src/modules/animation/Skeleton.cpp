/**
 * @file
 */

#include "Skeleton.h"
#include "core/Common.h"
#include "animation/BoneUtil.h"

namespace animation {

Bone& Skeleton::torsoBone(float scale) {
	Bone& torso = bone(BoneId::Torso);
	torso.scale = glm::vec3(_private::torsoScale * scale);
	torso.translation = glm::zero<glm::vec3>();
	torso.orientation = glm::quat_identity<float, glm::defaultp>();
	return torso;
}

void Skeleton::lerp(const Skeleton& previous, float dt) {
	const float factor = glm::min(1.0f, dt);
	for (int i = 0; i < std::enum_value(BoneId::Max); ++i) {
		const BoneId id = (BoneId)i;
		bone(id).lerp(previous.bone(id), factor);
	}
}

}
