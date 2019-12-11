/**
 * @file
 */

#include "Idle.h"
#include "animation/Animation.h"
#include "animation/BoneUtil.h"
#include "animation/animal/bird/BirdSkeleton.h"
#include "animation/animal/bird/BirdSkeletonAttribute.h"
#include "core/GLM.h"

namespace animation {
namespace animal {
namespace bird {
namespace idle {
void update(float animTime, BirdSkeleton &skeleton, const BirdSkeletonAttribute &skeletonAttr) {
	const float sine = glm::sin(animTime);
	const float cosine = glm::cos(animTime);

	Bone &head = skeleton.bone(BoneId::Head);
	head.translation = glm::vec3(skeletonAttr.headY);
	head.orientation = rotateYZ(sine * 0.1f, cosine * 0.05f);

	Bone &torso = skeleton.bone(BoneId::Torso);
	torso.translation = glm::zero<glm::vec3>();
	torso.orientation = glm::quat_identity<float, glm::defaultp>();

	skeleton.bone(BoneId::RightWing) = translate(skeletonAttr.wingRight, skeletonAttr.wingHeight, 0.0f);
	skeleton.bone(BoneId::LeftWing) = mirrorX(skeleton.bone(BoneId::RightWing));

	skeleton.bone(BoneId::RightFoot) = translate(skeletonAttr.footRight, skeletonAttr.footHeight, 0.0f);
	skeleton.bone(BoneId::LeftFoot) = mirrorX(skeleton.bone(BoneId::RightFoot));
}
}
}
}
}
