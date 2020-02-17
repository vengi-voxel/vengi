/**
 * @file
 */

#include "Run.h"
#include "animation/Animation.h"
#include "animation/BoneUtil.h"

namespace animation {
namespace animal {
namespace bird {
namespace run {
void update(float animTime, float velocity, BirdSkeleton &skeleton, const BirdSkeletonAttribute &skeletonAttr) {
	const float timeFactor = skeletonAttr.runTimeFactor;
	const float sine = glm::sin(animTime * timeFactor);
	const float cosine = glm::cos(animTime * timeFactor);
	const float cosineDouble = glm::cos(animTime * timeFactor * 2.0f);
	const float movement = sine * 0.35f;
	const glm::vec2 headLook(glm::cos(animTime) * 0.05f + glm::radians(10.0f), sine * 0.1f);

	velocity = glm::clamp(velocity / 10.0f, 0.1f, 2.5f);

	Bone &head = skeleton.headBone(skeletonAttr);
	head.translation = glm::vec3(0.0f, skeletonAttr.headY + cosine * 1.3f, 0.0f);
	head.orientation = rotateXY(headLook.x, headLook.y);

	const float bodyMoveY = cosine * 1.1f;
	Bone &body = skeleton.bodyBone(skeletonAttr);
	body.translation = glm::vec3(0.0f, skeletonAttr.bodyY + bodyMoveY, 0.0f);
	body.orientation = rotateY(movement);

	const float footAngle = cosine * 1.5f;
	const float footMoveY = cosineDouble * 0.5f;
	Bone &rightfoot = skeleton.footBone(BoneId::RightFoot, skeletonAttr);
	rightfoot.translation = glm::vec3(skeletonAttr.footRight, skeletonAttr.footHeight - footMoveY, 0.0f);
	rightfoot.orientation = rotateX(footAngle);

	Bone &leftfoot = skeleton.bone(BoneId::LeftFoot);
	leftfoot = mirrorX(rightfoot);
	leftfoot.orientation = rotateX(-footAngle);

	skeleton.bone(BoneId::RightWing) = translate(skeletonAttr.wingRight, skeletonAttr.wingY, 0.0f);
	skeleton.bone(BoneId::LeftWing) = mirrorX(skeleton.bone(BoneId::RightWing));

	Bone &torso = skeleton.torsoBone(skeletonAttr.scaler);
	torso.translation = glm::vec3(0.0f, 0.0f, sine * 0.04f);
	torso.orientation = rotateX(cosine * -0.01f);
}
}
}
}
}
