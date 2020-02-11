/**
 * @file
 */

#pragma once

#include "animation/Bone.h"
#include "animation/BoneId.h"
#include "animation/Skeleton.h"
#include "BirdSkeletonAttribute.h"
#include "core/GLM.h"

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

}
