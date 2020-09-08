/**
 * @file
 */

#include "Idle.h"
#include "animation/BoneUtil.h"
#include "animation/animal/bird/BirdSkeleton.h"
#include "animation/animal/bird/BirdSkeletonAttribute.h"
#include "core/GLMConst.h"

namespace animation {
namespace animal {
namespace bird {
namespace idle {
static void update(double animTime, BirdSkeleton &skeleton, const BirdSkeletonAttribute &skeletonAttr) {
	const float sine = glm::sin(animTime);
	const float cosine = glm::cos(animTime);

	Bone &head = skeleton.headBone(skeletonAttr);
	head.translation = glm::vec3(0.0f, skeletonAttr.headY, 0.0f);
	head.orientation = rotateYZ(sine * 0.1f, cosine * 0.05f);

	Bone &body = skeleton.bodyBone(skeletonAttr);
	body.translation = glm::vec3(0.0f, skeletonAttr.bodyY, 0.0f);
	body.orientation = glm::quat_identity<float, glm::defaultp>();

	skeleton.bone(BoneId::RightWing) = translate(skeletonAttr.wingRight, skeletonAttr.wingY, 0.0f);
	skeleton.bone(BoneId::LeftWing) = mirrorX(skeleton.bone(BoneId::RightWing));

	skeleton.bone(BoneId::RightFoot) = translate(skeletonAttr.footRight, skeletonAttr.footHeight, 0.0f);
	skeleton.bone(BoneId::LeftFoot) = mirrorX(skeleton.bone(BoneId::RightFoot));

	skeleton.torsoBone(skeletonAttr.scaler);
}
}
}
}
}

void animation_animal_bird_idle_update(double animTime, animation::BirdSkeleton* skeleton, const animation::BirdSkeletonAttribute* skeletonAttr) {
	animation::animal::bird::idle::update(animTime, *skeleton, *skeletonAttr);
}
