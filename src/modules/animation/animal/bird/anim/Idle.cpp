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
	head.scale = glm::vec3(skeletonAttr.headScale);
	head.translation = glm::vec3(0.0f, skeletonAttr.headY, 0.0f);
	head.orientation = rotateYZ(sine * 0.1f, cosine * 0.05f);

	skeleton.torsoBone(skeletonAttr.scaler);

	Bone &body = skeleton.bone(BoneId::Body);
	body.scale = glm::vec3(skeletonAttr.bodyScale);
	body.translation = glm::vec3(0.0f, skeletonAttr.bodyY, 0.0f);
	body.orientation = glm::quat_identity<float, glm::defaultp>();

	skeleton.bone(BoneId::RightWing) = translate(skeletonAttr.wingRight, skeletonAttr.wingY, 0.0f);
	skeleton.bone(BoneId::LeftWing) = mirrorX(skeleton.bone(BoneId::RightWing));

	skeleton.bone(BoneId::RightFoot) = translate(skeletonAttr.footRight, skeletonAttr.footY, 0.0f);
	skeleton.bone(BoneId::LeftFoot) = mirrorX(skeleton.bone(BoneId::RightFoot));
}
}
}
}
}
