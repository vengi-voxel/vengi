/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "operation/ExtrudeOperation.h"
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
 * In Place mode voxels are added outward from the selection by @c _parameters.depth steps.
 * The perpendicular "side walls" are filled to keep the model manifold.
 * In Erase mode the outermost selected layer facing the cursor is removed.
 *
 * @ingroup Brushes
 */
class ExtrudeBrush : public Brush, public ExtrudeOperation {
private:
	using Super = Brush;
	bool _active = false;

protected:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	ExtrudeBrush() : Super(BrushType::Extrude, ModifierType::Place, ModifierType::Place | ModifierType::Erase) {
	}
	ExtrudeBrush(const ExtrudeBrush &other);
	virtual ~ExtrudeBrush() = default;

	void onSceneChange() override;
	void reset() override;
	void onActivated() override;
	void preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) override;
	bool beginBrush(const BrushContext &ctx) override;
	void endBrush(BrushContext &ctx) override;
	bool active() const override;
	voxel::Region calcRegion(const BrushContext &ctx) const override;
	bool managesOwnSelection() const override {
		return true;
	}

	void clearOperation() {
		_history.clear();
		_cachedSelectedPositions.clear();
		_cachedWallCandidates.clear();
		_hasCachedSelection = false;
		_cachedSelBBoxValid = false;
		_parameters.depth = 0;
		_parameters.offsetU = 0;
		_parameters.offsetV = 0;
		_active = false;
		markDirty();
	}
	void setDepth(int depth);
	int depth() const {
		return _parameters.depth;
	}

	void setOffsetU(int offset);
	int offsetU() const {
		return _parameters.offsetU;
	}

	void setOffsetV(int offset);
	int offsetV() const {
		return _parameters.offsetV;
	}

	voxel::FaceNames face() const {
		return _parameters.face;
	}

	bool hasCachedSelection() const {
		return _hasCachedSelection;
	}

	void setFillWalls(bool fill) {
		if (_parameters.fillWalls == fill) {
			return;
		}
		_parameters.fillWalls = fill;
		markDirty();
	}
	bool fillWalls() const {
		return _parameters.fillWalls;
	}

	void handles(const BrushContext &ctx, core::DynamicArray<BrushGizmoState> &output) const override;
	bool hasHandle(const BrushContext &ctx) const;
	void describeHandle(const BrushContext &ctx, BrushGizmoState &state) const;
	bool editHandle(uint32_t handleId, BrushContext &ctx, const glm::mat4 &matrix,
						 const glm::mat4 &deltaMatrix, uint32_t operation) override;

};

} // namespace voxedit
