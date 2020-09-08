/**
 * @file
 */

#include "Skeleton.h"
#include "animation/BoneUtil.h"

namespace animation {

Skeleton::Skeleton() {
	_bones.fill(Bone());
}

void Skeleton::lerp(const Skeleton& previous, double deltaFrameSeconds) {
	for (int i = 0; i < core::enumVal(BoneId::Max); ++i) {
		const BoneId id = (BoneId)i;
		bone(id).lerp(previous.bone(id), deltaFrameSeconds);
	}
}

}
