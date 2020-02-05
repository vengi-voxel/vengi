/**
 * @file
 */

#include "Skeleton.h"
#include "core/Assert.h"
#include "animation/BoneUtil.h"
#include "core/Log.h"

namespace animation {

const Bone& Skeleton::bone(BoneId id) const {
	core_assert(id != BoneId::Max);
	return _bones[core::enumVal(id)];
}

Bone& Skeleton::bone(BoneId id) {
	core_assert(id != BoneId::Max);
	return _bones[core::enumVal(id)];
}

Bone& Skeleton::torsoBone(float scale) {
	Bone& torso = bone(BoneId::Torso);
	torso.scale = glm::vec3(_private::torsoScale * scale);
	torso.translation = glm::zero<glm::vec3>();
	torso.orientation = glm::quat_identity<float, glm::defaultp>();
	return torso;
}

void Skeleton::lerp(const Skeleton& previous, float dt) {
	const float factor = glm::min(1.0f, dt);
	for (int i = 0; i < core::enumVal(BoneId::Max); ++i) {
		const BoneId id = (BoneId)i;
		bone(id).lerp(previous.bone(id), factor);
	}
}

}
