/**
 * @file
 */

#include "Run.h"
#include "animation/Animation.h"
#include "animation/BoneUtil.h"

namespace animation {
namespace chr {
namespace swim {
void update(float animTime, float velocity, CharacterSkeleton &skeleton, const CharacterSkeletonAttribute &skeletonAttr) {
	const float timeFactor = skeletonAttr.runTimeFactor;
	const float sine = glm::sin(animTime * timeFactor);
	const float cosine = glm::cos(animTime * timeFactor);
	const float cosineSlow = glm::cos(animTime * timeFactor / 4.0f);
	const float movement = sine * 0.15f;
	const glm::vec2 headLook(glm::cos(animTime) * 0.1f + glm::radians(-30.0f), sine * 0.1f);

	velocity = glm::clamp(velocity / 20.0f, 0.1f, 2.5f);

	Bone &head = skeleton.headBone(skeletonAttr);
	head.translation = glm::vec3(0.0f, skeletonAttr.neckHeight + skeletonAttr.headY + cosine * 1.3f + 0.5f, -1.0f + skeletonAttr.neckForward);
	head.orientation = rotateXY(headLook.x, headLook.y);

	const float bodyMoveY = cosine * 0.5f;
	Bone &chest = skeleton.chestBone(skeletonAttr);
	chest.translation = glm::vec3(0.0f, skeletonAttr.chestY + bodyMoveY, 0.0f);
	chest.orientation = rotateY(movement);

	Bone &belt = skeleton.beltBone(skeletonAttr);
	belt.translation = glm::vec3(0.0f, skeletonAttr.beltY + bodyMoveY, 0.0f);
	belt.orientation = rotateY(movement);

	Bone &pants = skeleton.pantsBone(skeletonAttr);
	pants.translation = glm::vec3(0.0f, skeletonAttr.pantsY + bodyMoveY, 0.0f);
	pants.orientation = rotateY(movement);

	const float handAngle = sine * 0.05f;
	const float handMoveY = cosineSlow * 3.0f;
	const float handMoveX = glm::abs(cosineSlow * 4.0f);
	Bone &righthand = skeleton.handBone(BoneId::RightHand, skeletonAttr);
	righthand.translation = glm::vec3(skeletonAttr.handRight + 0.1f + handMoveX, handMoveY, skeletonAttr.handForward);
	righthand.orientation = rotateX(handAngle);

	Bone &lefthand = skeleton.handBone(BoneId::LeftHand, skeletonAttr);
	lefthand = mirrorXZ(righthand);
	lefthand.orientation = rotateX(-handAngle);

	const float footAngle = cosine * 0.5f;
	const float footMoveY = cosine * 0.001f;
	Bone &rightfoot = skeleton.footBone(BoneId::RightFoot, skeletonAttr);
	rightfoot.translation = glm::vec3(skeletonAttr.footRight, skeletonAttr.hipOffset - footMoveY, 0.0f);
	rightfoot.orientation = rotateX(footAngle);

	Bone &leftfoot = skeleton.bone(BoneId::LeftFoot);
	leftfoot = mirrorX(rightfoot);
	leftfoot.orientation = rotateX(-footAngle);

	Bone& tool = skeleton.bone(BoneId::Tool);
	tool.scale = glm::vec3(skeletonAttr.toolScale * 0.8f);
	tool.translation = glm::vec3(skeletonAttr.toolRight, skeletonAttr.pantsY, skeletonAttr.toolForward);
	tool.orientation = rotateYZ(glm::radians(-90.0f), glm::radians(110.0f));

	Bone &rightshoulder = skeleton.shoulderBone(BoneId::RightShoulder, skeletonAttr, rotateX(sine * 0.15f));

	Bone &torso = skeleton.torsoBone(skeletonAttr.scaler);
	torso.translation = glm::vec3(0.0f, 0.5f + sine * 0.04f, -skeletonAttr.beltY * torso.scale.z);
	torso.orientation = rotateXZ(glm::half_pi<float>() + -0.2f + cosine * 0.15f, cosine * 0.1f);

	skeleton.bone(BoneId::Glider) = zero();
	skeleton.bone(BoneId::LeftShoulder) = mirrorX(rightshoulder);
}
}
}
}
