/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "core/collection/DynamicArray.h"
#include "voxel/Face.h"
#include "voxel/Voxel.h"
#include <glm/vec3.hpp>

namespace voxedit {

/**
 * @brief Extrudes the current voxel selection along the clicked face normal
 *
 * In Place mode voxels are added outward from the selection by @c _depth steps.
 * Optionally the perpendicular "side walls" are filled to keep the model manifold.
 * In Erase mode the outermost selected layer facing the cursor is removed.
 *
 * @ingroup Brushes
 */
class ExtrudeBrush : public Brush {
private:
	using Super = Brush;
	voxel::FaceNames _face = voxel::FaceNames::Max;
	bool _active = false;
	bool _fillSides = false;
	int _depth = 0;
	struct HistoryEntry {
		glm::ivec3 pos;
		voxel::Voxel original;
	};
	// For each position overwritten by the current extrude session, stores the original voxel.
	// Cleared in reset() when the brush is deselected.
	core::DynamicArray<HistoryEntry> _history;

protected:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	ExtrudeBrush() : Super(BrushType::Extrude, ModifierType::Place, ModifierType::Place | ModifierType::Erase) {
	}
	virtual ~ExtrudeBrush() = default;

	void reset() override;
	bool beginBrush(const BrushContext &ctx) override;
	void endBrush(BrushContext &ctx) override;
	bool active() const override;
	voxel::Region calcRegion(const BrushContext &ctx) const override;

	void setDepth(int depth);
	int depth() const {
		return _depth;
	}

	voxel::FaceNames face() const {
		return _face;
	}

	void setFillSides(bool fill) {
		_fillSides = fill;
	}
	bool fillSides() const {
		return _fillSides;
	}
};

} // namespace voxedit
