/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "ExtrudeOperation.h"
#include "core/GLM.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicSet.h"
#include "voxel/DynamicVoxelArray.h"
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
class ExtrudeBrush : public Brush, public ExtrudeOperation {
private:
	using Super = Brush;
	bool _active = false;
	voxel::RawVolume *_lastVolume = nullptr;

protected:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	ExtrudeBrush() : Super(BrushType::Extrude, ModifierType::Place, ModifierType::Place | ModifierType::Erase) {
	}
	virtual ~ExtrudeBrush() = default;

	void onSceneChange() override;
	void reset() override;
	void onActivated() override;
	bool hasPendingChanges() const override;
	bool onDeactivated() override;
	voxel::Region revertChanges(voxel::RawVolume *volume) override;
	void preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) override;
	bool beginBrush(const BrushContext &ctx) override;
	void endBrush(BrushContext &ctx) override;
	bool active() const override;
	voxel::Region calcRegion(const BrushContext &ctx) const override;
	bool managesOwnSelection() const override {
		return true;
	}

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

	bool hasCachedSelection() const {
		return _hasCachedSelection;
	}

	void setFillWalls(bool fill) {
		if (_fillWalls == fill) {
			return;
		}
		_fillWalls = fill;
		markDirty();
	}
	bool fillWalls() const {
		return _fillWalls;
	}

	bool wantBrushGizmo(const BrushContext &ctx) const override;
	void brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const override;
	bool applyBrushGizmo(BrushContext &ctx, const glm::mat4 &matrix,
						 const glm::mat4 &deltaMatrix, uint32_t operation) override;

};

} // namespace voxedit
