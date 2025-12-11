/**
 * @file
 */

#pragma once

#include "core/collection/DynamicMap.h"
#include "scenegraph/FrameTransform.h"
#include "scenegraph/SceneGraphAnimation.h"

namespace scenegraph {

struct FrameTransformCacheKey {
	int32_t nodeId;
	FrameIndex frame;

	bool operator==(const FrameTransformCacheKey &other) const {
		return nodeId == other.nodeId && frame == other.frame;
	}
};
static_assert(sizeof(FrameTransformCacheKey) == sizeof(int) + sizeof(FrameIndex),
			  "Padding detected in FrameTransformCacheKey");

struct FrameTransformCacheKeyHasher {
	size_t operator()(const FrameTransformCacheKey &nf) const;
};

using FrameTransformCache = core::DynamicMap<FrameTransformCacheKey, FrameTransform, 531, FrameTransformCacheKeyHasher>;

} // namespace scenegraph
