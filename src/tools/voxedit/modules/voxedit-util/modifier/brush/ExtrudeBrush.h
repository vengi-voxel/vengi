/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "core/collection/DynamicArray.h"
#include "voxel/Face.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include <glm/vec3.hpp>

namespace voxedit {

/**
 * @brief Extrudes the current voxel selection along the clicked face normal
 *
 * In Place mode voxels are added outward from the selection by @c _depth steps.
 * The perpendicular "side walls" are filled to keep the model manifold.
 * In Erase mode the outermost selected layer facing the cursor is removed.
 *
 * @ingroup Brushes
 */
class ExtrudeBrush : public Brush {
private:
	using Super = Brush;
	voxel::FaceNames _face = voxel::FaceNames::Max;
	bool _active = false;
	int _depth = 0;
	int _offsetU = 0; // lateral offset along first perpendicular axis
	int _offsetV = 0; // lateral offset along second perpendicular axis
	struct HistoryEntry {
		glm::ivec3 pos;
		voxel::Voxel original;
	};
	// For each position overwritten by the current extrude session, stores the original voxel.
	// Cleared in reset() when the brush is deselected.
	// TODO: use a SparseVolume here?
	core::DynamicArray<HistoryEntry> _history;

	// Cached bounding box of selected (FlagOutline) voxels, computed in preExecute().
	// Used by calcRegion() to return a tight preview region instead of the full volume.
	voxel::Region _cachedSelBBox;
	bool _cachedSelBBoxValid = false;

protected:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	ExtrudeBrush() : Super(BrushType::Extrude, ModifierType::Place, ModifierType::Place | ModifierType::Erase) {
	}
	virtual ~ExtrudeBrush() = default;

	void onSceneChange() override;
	void reset() override;
	void preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) override;
	bool beginBrush(const BrushContext &ctx) override;
	void endBrush(BrushContext &ctx) override;
	bool active() const override;
	voxel::Region calcRegion(const BrushContext &ctx) const override;

	void setDepth(int depth);
	int depth() const {
		return _depth;
	}

	void setOffsetU(int offset);
	int offsetU() const {
		return _offsetU;
	}

	void setOffsetV(int offset);
	int offsetV() const {
		return _offsetV;
	}

	voxel::FaceNames face() const {
		return _face;
	}

	bool wantBrushGizmo(const BrushContext &ctx) const override;
	void brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const override;
	bool applyBrushGizmo(BrushContext &ctx, const glm::mat4 &matrix,
						 const glm::mat4 &deltaMatrix, uint32_t operation) override;

};

} // namespace voxedit
