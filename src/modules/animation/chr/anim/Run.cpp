/**
 * @file
 */

#include "Run.h"
#include "animation/Animation.h"
#include "animation/BoneUtil.h"

namespace animation {
namespace chr {
namespace run {
void update(float animTime, float velocity, CharacterSkeleton &skeleton, const CharacterSkeletonAttribute &skeletonAttr) {
	const float timeFactor = skeletonAttr.runTimeFactor;
	const float sine = glm::sin(animTime * timeFactor);
	const float cosine = glm::cos(animTime * timeFactor);
	const float cosineDouble = glm::cos(animTime * timeFactor * 2.0f);
	const float movement = sine * 0.35f;
	const glm::vec2 headLook(glm::cos(animTime) * 0.05f + glm::radians(10.0f), sine * 0.1f);

	velocity = glm::clamp(velocity / 10.0f, 0.1f, 2.5f);

	Bone &head = skeleton.headBone(skeletonAttr);
	head.translation = glm::vec3(0.0f, skeletonAttr.neckHeight + skeletonAttr.headY + cosine * 1.3f, -1.0f + skeletonAttr.neckForward);
	head.orientation = rotateXY(headLook.x, headLook.y);

	const float bodyMoveY = cosine * 1.1f;
	Bone &chest = skeleton.chestBone(skeletonAttr);
	chest.translation = glm::vec3(0.0f, skeletonAttr.chestY + bodyMoveY, 0.0f);
	chest.orientation = rotateY(movement);

	Bone &belt = skeleton.beltBone(skeletonAttr);
	belt.translation = glm::vec3(0.0f, skeletonAttr.beltY + bodyMoveY, 0.0f);
	belt.orientation = rotateY(movement);

	Bone &pants = skeleton.pantsBone(skeletonAttr);
	pants.translation = glm::vec3(0.0f, skeletonAttr.pantsY + bodyMoveY, 0.0f);
	pants.orientation = rotateY(movement);

	const float handAngle = sine * 0.2f;
	const float handMoveY = cosine;
	const float handMoveZ = cosine * 4.0f;
	Bone &righthand = skeleton.handBone(BoneId::RightHand, skeletonAttr);
	righthand.translation = glm::vec3(skeletonAttr.handRight + cosineDouble, handMoveY, skeletonAttr.handForward + handMoveZ);
	righthand.orientation = rotateX(handAngle);

	Bone &lefthand = skeleton.handBone(BoneId::LeftHand, skeletonAttr);
	lefthand = mirrorXZ(righthand);
	lefthand.orientation = rotateX(-handAngle);

	const float footAngle = cosine * 1.5f;
	const float footMoveY = cosineDouble * 0.5f;
	Bone &rightfoot = skeleton.footBone(BoneId::RightFoot, skeletonAttr);
	rightfoot.translation = glm::vec3(skeletonAttr.footRight, skeletonAttr.hipOffset - footMoveY, 0.0f);
	rightfoot.orientation = rotateX(footAngle);

	Bone &leftfoot = skeleton.bone(BoneId::LeftFoot);
	leftfoot = mirrorX(rightfoot);
	leftfoot.orientation = rotateX(-footAngle);

	skeleton.toolBone(skeletonAttr, cosine * 0.25f);

	Bone &rightshoulder = skeleton.shoulderBone(BoneId::RightShoulder, skeletonAttr, rotateX(sine * 0.15f));

	Bone &torso = skeleton.torsoBone(skeletonAttr.scaler);
	torso.translation = glm::vec3(0.0f, 0.0f, sine * 0.04f);
	torso.orientation = rotateX(cosine * 0.1f);

	skeleton.bone(BoneId::Glider) = zero();
	skeleton.bone(BoneId::LeftShoulder) = mirrorX(rightshoulder);
}
}
}
}
