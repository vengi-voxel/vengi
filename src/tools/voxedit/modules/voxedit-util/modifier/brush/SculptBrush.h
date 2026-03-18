/**
 * @file
 */

#pragma once

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
	SmoothAdditive,
	SmoothErode,
	SmoothGaussian,
	BridgeGap,
	SquashToPlane,

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
	int _heightThreshold = 1;
	bool _preserveTopHeight = false;
	int _trimPerStep = 1;
	int _kernelSize = 4;
	float _sigma = 4.0f;
	voxel::FaceNames _flattenFace = voxel::FaceNames::Max;
	int _squashPlaneCoord = 0;
	bool _active = false;
	bool _hasSnapshot = false;
	bool _paramsDirty = true;

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
	void applySculpt(ModifierVolumeWrapper &wrapper, const BrushContext &ctx);
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

	static bool modeNeedsFace(SculptMode mode) {
		return mode == SculptMode::Flatten || mode == SculptMode::SmoothAdditive || mode == SculptMode::SmoothErode || mode == SculptMode::SmoothGaussian || mode == SculptMode::SquashToPlane;
	}

	SculptMode sculptMode() const {
		return _sculptMode;
	}

	void setSculptMode(SculptMode mode) {
		const bool needsFace = modeNeedsFace(mode);
		const bool hadFace = modeNeedsFace(_sculptMode);
		if (needsFace && !hadFace) {
			_flattenFace = voxel::FaceNames::Max;
		}
		if (mode == SculptMode::SmoothGaussian && _sculptMode != SculptMode::SmoothGaussian) {
			_iterations = 3;
		}
		_sculptMode = mode;
		_paramsDirty = true;
	}

	float strength() const {
		return _strength;
	}

	void setStrength(float strength) {
		_strength = glm::clamp(strength, 0.0f, 1.0f);
		_paramsDirty = true;
	}

	int iterations() const {
		return _iterations;
	}

	void setIterations(int iterations) {
		_iterations = glm::clamp(iterations, 1, MaxFlattenIterations);
		_paramsDirty = true;
	}

	bool hasSnapshot() const {
		return _hasSnapshot;
	}

	voxel::FaceNames flattenFace() const {
		return _flattenFace;
	}

	static constexpr int MaxHeightThreshold = 10;

	int heightThreshold() const {
		return _heightThreshold;
	}

	void setHeightThreshold(int threshold) {
		_heightThreshold = glm::clamp(threshold, 1, MaxHeightThreshold);
		_paramsDirty = true;
	}

	bool preserveTopHeight() const {
		return _preserveTopHeight;
	}

	void setPreserveTopHeight(bool preserve) {
		_preserveTopHeight = preserve;
		_paramsDirty = true;
	}

	static constexpr int MaxTrimPerStep = 16;

	int trimPerStep() const {
		return _trimPerStep;
	}

	void setTrimPerStep(int value) {
		_trimPerStep = glm::clamp(value, 1, MaxTrimPerStep);
		_paramsDirty = true;
	}

	static constexpr int MaxKernelSize = 7;

	int kernelSize() const {
		return _kernelSize;
	}

	void setKernelSize(int size) {
		_kernelSize = glm::clamp(size, 1, MaxKernelSize);
		_paramsDirty = true;
	}

	static constexpr float MaxSigma = 8.0f;
	static constexpr float MinSigma = 0.1f;

	float sigma() const {
		return _sigma;
	}

	void setSigma(float sigma) {
		_sigma = glm::clamp(sigma, MinSigma, MaxSigma);
		_paramsDirty = true;
	}
};

} // namespace voxedit
