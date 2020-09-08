/**
 * @file
 */

#pragma once

#include "animation/Bone.h"
#include "animation/BoneId.h"
#include "animation/Skeleton.h"
#include "BirdSkeletonAttribute.h"
#include <glm/fwd.hpp>

namespace animation {

/**
 * @brief The bones of the @c Bird
 * @ingroup Animation
 */
class BirdSkeleton : public Skeleton {
public:
	void update(const AnimationSettings& settings, glm::mat4 (&bones)[shader::SkeletonShaderConstants::getMaxBones()]) const override;

	Bone& footBone(BoneId id, const BirdSkeletonAttribute& skeletonAttr);
	Bone& bodyBone(const BirdSkeletonAttribute& skeletonAttr);
	Bone& headBone(const BirdSkeletonAttribute& skeletonAttr);
};

inline Bone& BirdSkeleton::footBone(BoneId id, const BirdSkeletonAttribute& skeletonAttr) {
	Bone& foot = bone(id);
	foot.scale = glm::one<glm::vec3>();
	return foot;
}

inline Bone& BirdSkeleton::bodyBone(const BirdSkeletonAttribute& skeletonAttr) {
	Bone& body = bone(BoneId::Body);
	body.scale = glm::vec3(skeletonAttr.bodyScale);
	return body;
}

inline Bone& BirdSkeleton::headBone(const BirdSkeletonAttribute& skeletonAttr) {
	Bone& head = bone(BoneId::Head);
	head.scale = glm::vec3(skeletonAttr.headScale);
	return head;
}

}
