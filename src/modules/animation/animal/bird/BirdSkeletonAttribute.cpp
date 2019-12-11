/**
 * @file
 */

#include "BirdSkeletonAttribute.h"

namespace animation {

#define SKELETONATTRIBUTEBIRD(member) SKELETONATTRIBUTE(struct BirdSkeletonAttribute, member)

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif

static const SkeletonAttributeMeta BirdSkeletonAttributeMetaArray[] = {
	SKELETONATTRIBUTEBIRD(scaler),
	SKELETONATTRIBUTEBIRD(origin),
	SKELETONATTRIBUTEBIRD(invisibleLegHeight),
	SKELETONATTRIBUTEBIRD(headHeight),
	SKELETONATTRIBUTEBIRD(footHeight),
	SKELETONATTRIBUTEBIRD(footRight),
	SKELETONATTRIBUTEBIRD(wingHeight),
	SKELETONATTRIBUTEBIRD(wingRight),
	SKELETONATTRIBUTE_END
};

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#undef SKELETONATTRIBUTEBIRD

BirdSkeletonAttribute::BirdSkeletonAttribute() :
		SkeletonAttribute(SkeletonAttributeType::Bird, BirdSkeletonAttributeMetaArray) {
}

}
