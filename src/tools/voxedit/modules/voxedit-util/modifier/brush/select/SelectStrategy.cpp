/**
 * @file
 */

#include "SelectStrategy.h"
#include "voxedit-util/modifier/brush/Brush.h"

namespace voxedit {
namespace select {

voxel::Region Strategy::calcRegion(const BrushContext &ctx, const AABBBrushState &state) const {
	return ctx.targetVolumeRegion;
}

bool Strategy::needsAdditionalAction(const BrushContext &ctx) const {
	return false;
}

bool Strategy::beginBrush(const BrushContext &ctx, const AABBBrushState &state) {
	return false;
}

void Strategy::endBrush(BrushContext &ctx) {
}

void Strategy::abort(BrushContext &ctx) {
}

void Strategy::reset() {
}

void Strategy::update(const BrushContext &ctx, double nowSeconds) {
}

bool Strategy::active() const {
	return false;
}

} // namespace select
} // namespace voxedit
