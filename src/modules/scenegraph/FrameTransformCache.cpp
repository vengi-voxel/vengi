/**
 * @file
 */

#include "FrameTransformCache.h"
#include "core/Hash.h"

namespace scenegraph {

size_t FrameTransformCacheKeyHasher::operator()(const FrameTransformCacheKey &nf) const {
	return (size_t)core::hash(&nf, sizeof(nf));
}

} // namespace scenegraph
