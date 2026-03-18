/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "core/GLM.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "voxel/Face.h"
#include "voxel/SparseVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeSculpt.h"

#include <glm/vec3.hpp>

namespace voxel {
class RawVolume;
}

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
	Reskin,

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

	// Reskin parameters
	voxelutil::ReskinConfig _reskinConfig;
	const voxel::RawVolume *_skinVolume = nullptr;
	core::ScopedPtr<voxel::RawVolume> _ownedSkinVolume;
	core::String _skinFilePath;

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
		return mode == SculptMode::Flatten || mode == SculptMode::SmoothAdditive || mode == SculptMode::SmoothErode || mode == SculptMode::SmoothGaussian || mode == SculptMode::SquashToPlane || mode == SculptMode::Reskin;
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

	// Reskin accessors
	static constexpr int MaxReskinDepth = 32;

	const voxelutil::ReskinConfig &reskinConfig() const {
		return _reskinConfig;
	}

	void setReskinMode(voxelutil::ReskinMode mode) {
		_reskinConfig.mode = mode;
		_paramsDirty = true;
	}

	void setReskinFollow(voxelutil::ReskinFollow follow) {
		_reskinConfig.follow = follow;
		_paramsDirty = true;
	}

	void setReskinAnchor(voxelutil::ReskinAnchor anchor) {
		_reskinConfig.anchor = anchor;
		_paramsDirty = true;
	}

	void setReskinRotation(voxelutil::ReskinRotation rotation) {
		_reskinConfig.rotation = rotation;
		_paramsDirty = true;
	}

	void setReskinTile(voxelutil::ReskinTile tile) {
		_reskinConfig.tile = tile;
		_paramsDirty = true;
	}

	void setReskinMirrorU(bool mirror) {
		_reskinConfig.mirrorU = mirror;
		_paramsDirty = true;
	}

	void setReskinMirrorV(bool mirror) {
		_reskinConfig.mirrorV = mirror;
		_paramsDirty = true;
	}

	void setReskinOffsetU(int offset) {
		_reskinConfig.offsetU = offset;
		_paramsDirty = true;
	}

	void setReskinOffsetV(int offset) {
		_reskinConfig.offsetV = offset;
		_paramsDirty = true;
	}

	void setReskinSkinDepth(int depth) {
		_reskinConfig.skinDepth = glm::clamp(depth, 1, MaxReskinDepth);
		_paramsDirty = true;
	}

	void setReskinZOffset(int offset) {
		_reskinConfig.zOffset = glm::clamp(offset, -MaxReskinDepth, MaxReskinDepth);
		_paramsDirty = true;
	}

	void setReskinInvertSkin(bool invert) {
		_reskinConfig.invertSkin = invert;
		_paramsDirty = true;
	}

	void setReskinSkinUpAxis(math::Axis axis) {
		_reskinConfig.skinUpAxis = axis;
		// Re-populate skin depth for the new axis
		if (_skinVolume != nullptr) {
			setSkinVolume(_skinVolume);
		}
		_paramsDirty = true;
	}

	/**
	 * @brief Set the skin volume for reskin mode.
	 * @param skinVolume The skin volume to apply (not owned, must outlive the brush operation).
	 */
	void setSkinVolume(const voxel::RawVolume *skinVolume) {
		_skinVolume = skinVolume;
		// Auto-populate skin depth from the skin volume's depth along the configured up axis
		if (skinVolume != nullptr) {
			const voxel::Region &sr = skinVolume->region();
			const int upIdx = math::getIndexForAxis(_reskinConfig.skinUpAxis);
			const int depthExtent = sr.getUpperCorner()[upIdx] - sr.getLowerCorner()[upIdx] + 1;
			_reskinConfig.skinDepth = glm::clamp(depthExtent, 1, MaxReskinDepth);
		}
		_paramsDirty = true;
	}

	/**
	 * @brief Set an owned skin volume loaded from a file. The brush takes ownership.
	 */
	void setOwnedSkinVolume(voxel::RawVolume *skinVolume, const core::String &filePath) {
		_ownedSkinVolume = skinVolume;
		_skinFilePath = filePath;
		setSkinVolume(skinVolume);
	}

	const voxel::RawVolume *skinVolume() const {
		return _skinVolume;
	}

	const core::String &skinFilePath() const {
		return _skinFilePath;
	}
};

} // namespace voxedit
