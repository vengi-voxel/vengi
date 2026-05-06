/**
 * @file
 */

#pragma once

#include "voxedit-util/modifier/SceneModifiedFlags.h"
#include "voxel/Face.h"
#include "voxel/Region.h"
#include <glm/vec3.hpp>

namespace scenegraph {
class SceneGraph;
}

namespace voxedit {

struct BrushContext;
class ModifierVolumeWrapper;

namespace select {

/**
 * @brief AABB state passed from the brush to strategies that need it
 */
struct AABBBrushState {
	bool aabbMode = false;
	voxel::FaceNames aabbFace = voxel::FaceNames::Max;
	glm::ivec3 aabbFirstPos{0};
	glm::ivec3 cursorPosition{0};
	int radius = 0;
};

/**
 * @brief Base class for select mode strategies
 * @ingroup Brushes
 */
class Strategy {
public:
	/** Scene modified flags set by the strategy - read by SelectBrush after method calls */
	SceneModifiedFlags _modifiedFlags = SceneModifiedFlags::All;

	virtual ~Strategy() = default;

	virtual void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						  const voxel::Region &region, const AABBBrushState &state) = 0;

	virtual voxel::Region calcRegion(const BrushContext &ctx, const AABBBrushState &state) const;
	virtual bool needsAdditionalAction(const BrushContext &ctx) const;
	virtual bool beginBrush(const BrushContext &ctx, const AABBBrushState &state);
	virtual void endBrush(BrushContext &ctx);
	virtual void abort(BrushContext &ctx);
	virtual void reset();
	virtual void update(const BrushContext &ctx, double nowSeconds);
	virtual bool active() const;
};

} // namespace select
} // namespace voxedit
