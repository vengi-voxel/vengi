/**
 * @file
 */

#pragma once

#include "app/I18N.h"
#include "Brush.h"
#include "core/GLM.h"
#include "voxel/Face.h"
#include "voxel/SparseVolume.h"
#include "voxel/Voxel.h"

#include <glm/vec3.hpp>

namespace voxedit {

enum class SculptMode : uint8_t {
	Erode,
	Grow,
	Flatten,

	Max
};

/**
 * @brief Sculpts selected voxels by smoothing surface irregularities
 *
 * Captures selected voxels as a snapshot on activation. Each parameter change
 * re-applies the sculpt operation from the original snapshot so changes are
 * absolute and reversible. On finalize the result is committed to the undo stack.
 *
 * @ingroup Brushes
 */
class SculptBrush : public Brush {
private:
	using Super = Brush;

	SculptMode _sculptMode = SculptMode::Erode;
	float _strength = 0.5f;
	int _iterations = 1;
	voxel::FaceNames _flattenFace = voxel::FaceNames::Max;
	bool _active = false;
	bool _hasSnapshot = false;

	// Original selected voxels captured at brush activation
	voxel::SparseVolume _snapshot;
	// Selection bounding box at capture time
	voxel::Region _snapshotRegion;
	// Volume region lower corner at snapshot capture time (to detect region shifts)
	glm::ivec3 _capturedVolumeLower{0};

	// Per-generate bookkeeping: tracks positions written during a single generate()
	// call so the previous state can be restored before re-applying.
	voxel::SparseVolume _history;

	// Cached region for preview
	voxel::Region _cachedRegion;
	bool _cachedRegionValid = false;

	void captureSnapshot(const voxel::RawVolume *volume, const voxel::Region &volRegion);
	void adjustSnapshotForRegionShift(const glm::ivec3 &delta);
	void applySmooth(ModifierVolumeWrapper &wrapper, const BrushContext &ctx);
	void saveToHistory(voxel::RawVolume *vol, const glm::ivec3 &pos);
	void writeVoxel(ModifierVolumeWrapper &wrapper, const glm::ivec3 &pos, const voxel::Voxel &voxel);

protected:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	static constexpr int MaxIterations = 10;
	static constexpr int MaxFlattenIterations = 64;
	SculptBrush() : Super(BrushType::Sculpt, ModifierType::Override, ModifierType::Override) {
		_history.setStoreEmptyVoxels(true);
	}
	virtual ~SculptBrush() = default;

	void onSceneChange() override;
	void reset() override;
	void onActivated() override;
	bool onDeactivated() override;
	void preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) override;
	bool beginBrush(const BrushContext &ctx) override;
	void endBrush(BrushContext &ctx) override;
	bool active() const override;
	voxel::Region calcRegion(const BrushContext &ctx) const override;
	bool managesOwnSelection() const override {
		return true;
	}

	SculptMode sculptMode() const {
		return _sculptMode;
	}

	void setSculptMode(SculptMode mode) {
		if (mode == SculptMode::Flatten && _sculptMode != SculptMode::Flatten) {
			_flattenFace = voxel::FaceNames::Max;
		}
		_sculptMode = mode;
	}

	float strength() const {
		return _strength;
	}

	void setStrength(float strength) {
		_strength = glm::clamp(strength, 0.0f, 1.0f);
	}

	int iterations() const {
		return _iterations;
	}

	void setIterations(int iterations) {
		_iterations = glm::clamp(iterations, 1, MaxFlattenIterations);
	}

	bool hasSnapshot() const {
		return _hasSnapshot;
	}

	voxel::FaceNames flattenFace() const {
		return _flattenFace;
	}
};

} // namespace voxedit
