/**
 * @file
 */

#include "CharacterSkeletonAttribute.h"

namespace animation {

#define SKELETONATTRIBUTECHR(member) SKELETONATTRIBUTE(struct CharacterSkeletonAttribute, member)

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif

static const SkeletonAttributeMeta ChrSkeletonAttributeMetaArray[] = {
	SKELETONATTRIBUTECHR(scaler),
	SKELETONATTRIBUTECHR(toolRight),
	SKELETONATTRIBUTECHR(toolForward),
	SKELETONATTRIBUTECHR(toolScale),
	SKELETONATTRIBUTECHR(neckRight),
	SKELETONATTRIBUTECHR(neckForward),
	SKELETONATTRIBUTECHR(neckHeight),
	SKELETONATTRIBUTECHR(headScale),
	SKELETONATTRIBUTECHR(handRight),
	SKELETONATTRIBUTECHR(handForward),
	SKELETONATTRIBUTECHR(shoulderRight),
	SKELETONATTRIBUTECHR(shoulderForward),
	SKELETONATTRIBUTECHR(runTimeFactor),
	SKELETONATTRIBUTECHR(jumpTimeFactor),
	SKELETONATTRIBUTECHR(idleTimeFactor),
	SKELETONATTRIBUTECHR(shoulderScale),
	SKELETONATTRIBUTECHR(hipOffset),
	SKELETONATTRIBUTECHR(origin),
	SKELETONATTRIBUTECHR(footHeight),
	SKELETONATTRIBUTECHR(invisibleLegHeight),
	SKELETONATTRIBUTECHR(pantsHeight),
	SKELETONATTRIBUTECHR(beltHeight),
	SKELETONATTRIBUTECHR(chestHeight),
	SKELETONATTRIBUTECHR(gliderOffset),
	SKELETONATTRIBUTECHR(glidingForward),
	SKELETONATTRIBUTECHR(glidingUpwards),
	SKELETONATTRIBUTECHR(headHeight),
	SKELETONATTRIBUTECHR(footRight),

	SKELETONATTRIBUTECHR(footY),
	SKELETONATTRIBUTECHR(pantsY),
	SKELETONATTRIBUTECHR(beltY),
	SKELETONATTRIBUTECHR(chestY),
	SKELETONATTRIBUTECHR(headY),
	SKELETONATTRIBUTECHR(gliderY),

	SKELETONATTRIBUTE_END
};

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#undef SKELETONATTRIBUTECHR

CharacterSkeletonAttribute::CharacterSkeletonAttribute() :
		SkeletonAttribute(SkeletonAttributeType::Character, ChrSkeletonAttributeMetaArray) {
}

}
