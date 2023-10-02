/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
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

class Brush : public core::IComponent {
protected:
	bool _dirty = false;
	void markDirty();

public:
	virtual bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						 const BrushContext &ctx) = 0;
	virtual void reset();
	virtual void update(const BrushContext &ctx, double nowSeconds);

	/**
	 * @brief Determine whether the brush should get rendered
	 */
	virtual bool active() const;
	bool dirty() const;
	void markClean();
	bool init() override;
	void shutdown() override;
};

} // namespace voxedit
