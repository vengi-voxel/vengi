/**
 * @file
 */

#pragma once

#include "voxel/Face.h"
#include "voxel/Region.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace scenegraph {
class SceneGraph;
}

namespace voxedit {

struct BrushGizmoState;
struct BrushContext;
class ModifierVolumeWrapper;

namespace select {

/**
 * @brief AABB state passed from the brush to strategies that need it
 */
struct AABBBrushState {
	bool boxMode = false;
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
	virtual ~Strategy() = default;

	virtual bool isSimplePreview() const {
		return false;
	}

	virtual bool evaluate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						  const voxel::Region &region, const AABBBrushState &state) {
		generate(sceneGraph, wrapper, ctx, region, state);
		return true;
	}

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

	virtual bool hasHandle(const BrushContext &ctx, const AABBBrushState &state) const {
		return false;
	}

	virtual void describeHandle(const BrushContext &ctx, const AABBBrushState &state, BrushGizmoState &gizmoState) const {
	}

	virtual bool editHandle(uint32_t handleId, BrushContext &ctx, const glm::mat4 &matrix, const glm::mat4 &deltaMatrix,
								 uint32_t operation) {
		return false;
	}
};

} // namespace select
} // namespace voxedit
