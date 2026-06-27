/**
 * @file
 */

#pragma once

#include "app/I18NMarkers.h"
#include "core/ArrayLength.h"
#include "voxedit-util/modifier/SceneModifiedFlags.h"
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
 * @brief How a line/edge tracks the volume between its endpoints.
 *
 * FollowSurface keeps the path straight in the face plane but snaps the depth to the
 * surface, so the thickness always reaches it on stepped/sloped geometry. Straight draws a
 * plain 3D chord between the endpoints and only selects solid voxels it actually passes
 * through.
 */
enum class PathMode : uint8_t { FollowSurface, Straight, Max };

// clang-format off
static constexpr const char *PathModeStr[] = {
	NC_("PathMode", "Follow surface"),
	NC_("PathMode", "Straight")};
// clang-format on
static_assert(lengthof(PathModeStr) == (int)PathMode::Max, "PathModeStr size mismatch");

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
	/** Scene modified flags set by the strategy - read by SelectBrush after method calls */
	SceneModifiedFlags _modifiedFlags = SceneModifiedFlags::All;

	virtual ~Strategy() = default;

	virtual bool isSimplePreview() const {
		return false;
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

	virtual bool wantBrushGizmo(const BrushContext &ctx) const {
		return false;
	}

	virtual void brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const {
	}

	virtual bool applyBrushGizmo(BrushContext &ctx, const glm::mat4 &matrix, const glm::mat4 &deltaMatrix,
								 uint32_t operation) {
		return false;
	}
};

} // namespace select
} // namespace voxedit
