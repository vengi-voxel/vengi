/**
 * @file
 */

#pragma once

#include "BrushType.h"
#include "core/DirtyState.h"
#include "core/IComponent.h"
#include "math/Axis.h"
#include "voxel/Face.h"
#include "voxel/Voxel.h"

namespace scenegraph {
class SceneGraph;
}

namespace voxedit {

class ModifierVolumeWrapper;

struct BrushContext {
	/** the voxel that should get placed */
	voxel::Voxel cursorVoxel;
	/** existing voxel under the cursor */
	voxel::Voxel hitCursorVoxel;
	/** the voxel where the cursor is - can be air */
	voxel::Voxel voxelAtCursor;

	glm::ivec3 referencePos{0};
	glm::ivec3 cursorPosition{0};
	/** the face where the trace hit */
	voxel::FaceNames cursorFace = voxel::FaceNames::Max;
	math::Axis lockedAxis = math::Axis::None;

	int gridResolution = 1;
};

class Brush : public core::IComponent, public core::DirtyState {
protected:
	const BrushType _brushType;
	Brush(BrushType brushType) : _brushType(brushType) {
	}

public:
	virtual bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						 const BrushContext &ctx) = 0;
	virtual void reset();
	virtual void update(const BrushContext &ctx, double nowSeconds);
	core::String name() const;
	BrushType type() const;

	/**
	 * @brief Determine whether the brush should get rendered
	 */
	virtual bool active() const;
	bool init() override;
	void shutdown() override;
};

inline BrushType Brush::type() const {
	return _brushType;
}

inline core::String Brush::name() const {
	return BrushTypeStr[(int)_brushType];
}

} // namespace voxedit
