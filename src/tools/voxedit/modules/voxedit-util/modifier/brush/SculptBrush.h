/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "SnapshotHelper.h"
#include "core/GLM.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "voxel/Face.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeSculpt.h"

#include <climits>
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
	ExtendPlane,
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
	bool _paramsDirty = true;

	// ExtendPlane parameters
	int _removeAboveDepth = 0;
	bool _extendOnly = true;
	int _brushRadius = 3;
	float _planeGradU = 0.0f;
	float _planeGradV = 0.0f;
	int _planeSeedU = 0;
	int _planeSeedV = 0;
	int _planeSeedH = 0;
	int _planeUAxis = 0;
	int _planeVAxis = 2;
	int _planeHeightAxis = 1;
	bool _planeFitted = false;
	glm::ivec3 _lastPaintPos{INT_MIN};

	// Reskin parameters
	voxelutil::ReskinConfig _reskinConfig;
	const voxel::RawVolume *_skinVolume = nullptr;
	core::ScopedPtr<voxel::RawVolume> _ownedSkinVolume;
	core::String _skinFilePath;

	SnapshotHelper _snapshotHelper;

	// Cached region for preview
	voxel::Region _cachedRegion;
	bool _cachedRegionValid = false;

	void applySculpt(ModifierVolumeWrapper &wrapper, const BrushContext &ctx);
	void fitPlaneFromSnapshot();
	void paintExtendPlane(ModifierVolumeWrapper &wrapper, const BrushContext &ctx);

protected:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	static constexpr int MaxIterations = 250;
	static constexpr int MaxFlattenIterations = 250;
	SculptBrush() : Super(BrushType::Sculpt, ModifierType::Override, ModifierType::Override) {
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
	bool managesOwnSelection() const override;
	bool wantsContinuousExecution() const override;

	static bool modeNeedsFace(SculptMode mode);

	SculptMode sculptMode() const;
	void setSculptMode(SculptMode mode);

	float strength() const;
	void setStrength(float strength);

	int iterations() const;
	void setIterations(int iterations);

	bool hasSnapshot() const;

	voxel::FaceNames flattenFace() const;

	static constexpr int MaxHeightThreshold = 10;
	int heightThreshold() const;
	void setHeightThreshold(int threshold);

	bool preserveTopHeight() const;
	void setPreserveTopHeight(bool preserve);

	static constexpr int MaxTrimPerStep = 16;
	int trimPerStep() const;
	void setTrimPerStep(int value);

	static constexpr int MaxKernelSize = 7;
	int kernelSize() const;
	void setKernelSize(int size);

	static constexpr float MaxSigma = 8.0f;
	static constexpr float MinSigma = 0.1f;
	float sigma() const;
	void setSigma(float sigma);

	// ExtendPlane accessors
	static constexpr int MaxBrushRadius = 32;
	static constexpr int MaxRemoveAboveDepth = 32;
	int removeAboveDepth() const;
	void setRemoveAboveDepth(int depth);
	bool extendOnly() const;
	void setExtendOnly(bool extend);
	int brushRadius() const;
	void setBrushRadius(int radius);
	bool planeFitted() const;

	// Reskin accessors
	static constexpr int MaxReskinDepth = 32;
	const voxelutil::ReskinConfig &reskinConfig() const;
	void setReskinMode(voxelutil::ReskinMode mode);
	void setReskinFollow(voxelutil::ReskinFollow follow);
	void setReskinAnchor(voxelutil::ReskinAnchor anchor);
	void setReskinRotation(voxelutil::ReskinRotation rotation);
	void setReskinTile(voxelutil::ReskinTile tile);
	void setReskinMirrorU(bool mirror);
	void setReskinMirrorV(bool mirror);
	void setReskinOffsetU(int offset);
	void setReskinOffsetV(int offset);
	void setReskinSkinDepth(int depth);
	void setReskinZOffset(int offset);
	void setReskinInvertSkin(bool invert);
	void setReskinSkinUpAxis(math::Axis axis);

	/**
	 * @brief Set the skin volume for reskin mode.
	 * @param skinVolume The skin volume to apply (not owned, must outlive the brush operation).
	 */
	void setSkinVolume(const voxel::RawVolume *skinVolume);

	/**
	 * @brief Set an owned skin volume loaded from a file. The brush takes ownership.
	 */
	void setOwnedSkinVolume(voxel::RawVolume *skinVolume, const core::String &filePath);

	const voxel::RawVolume *skinVolume() const;
	const core::String &skinFilePath() const;
};

inline bool SculptBrush::managesOwnSelection() const {
	return true;
}

inline bool SculptBrush::wantsContinuousExecution() const {
	return _sculptMode == SculptMode::ExtendPlane && _planeFitted;
}

inline bool SculptBrush::modeNeedsFace(SculptMode mode) {
	return mode == SculptMode::Flatten || mode == SculptMode::SmoothAdditive || mode == SculptMode::SmoothErode ||
		   mode == SculptMode::SmoothGaussian || mode == SculptMode::SquashToPlane || mode == SculptMode::ExtendPlane ||
		   mode == SculptMode::Reskin;
}

inline SculptMode SculptBrush::sculptMode() const {
	return _sculptMode;
}

inline float SculptBrush::strength() const {
	return _strength;
}

inline void SculptBrush::setStrength(float strength) {
	_strength = glm::clamp(strength, 0.0f, 1.0f);
	_paramsDirty = true;
}

inline int SculptBrush::iterations() const {
	return _iterations;
}

inline void SculptBrush::setIterations(int iterations) {
	_iterations = glm::clamp(iterations, 1, MaxFlattenIterations);
	_paramsDirty = true;
}

inline bool SculptBrush::hasSnapshot() const {
	return _snapshotHelper.hasSnapshot();
}

inline voxel::FaceNames SculptBrush::flattenFace() const {
	return _flattenFace;
}

inline int SculptBrush::heightThreshold() const {
	return _heightThreshold;
}

inline void SculptBrush::setHeightThreshold(int threshold) {
	_heightThreshold = glm::clamp(threshold, 1, MaxHeightThreshold);
	_paramsDirty = true;
}

inline bool SculptBrush::preserveTopHeight() const {
	return _preserveTopHeight;
}

inline void SculptBrush::setPreserveTopHeight(bool preserve) {
	_preserveTopHeight = preserve;
	_paramsDirty = true;
}

inline int SculptBrush::trimPerStep() const {
	return _trimPerStep;
}

inline void SculptBrush::setTrimPerStep(int value) {
	_trimPerStep = glm::clamp(value, 1, MaxTrimPerStep);
	_paramsDirty = true;
}

inline int SculptBrush::kernelSize() const {
	return _kernelSize;
}

inline void SculptBrush::setKernelSize(int size) {
	_kernelSize = glm::clamp(size, 1, MaxKernelSize);
	_paramsDirty = true;
}

inline float SculptBrush::sigma() const {
	return _sigma;
}

inline void SculptBrush::setSigma(float sigma) {
	_sigma = glm::clamp(sigma, MinSigma, MaxSigma);
	_paramsDirty = true;
}

inline const voxelutil::ReskinConfig &SculptBrush::reskinConfig() const {
	return _reskinConfig;
}

inline void SculptBrush::setReskinMode(voxelutil::ReskinMode mode) {
	_reskinConfig.mode = mode;
	_paramsDirty = true;
}

inline void SculptBrush::setReskinFollow(voxelutil::ReskinFollow follow) {
	_reskinConfig.follow = follow;
	_paramsDirty = true;
}

inline void SculptBrush::setReskinAnchor(voxelutil::ReskinAnchor anchor) {
	_reskinConfig.anchor = anchor;
	_paramsDirty = true;
}

inline void SculptBrush::setReskinRotation(voxelutil::ReskinRotation rotation) {
	_reskinConfig.rotation = rotation;
	_paramsDirty = true;
}

inline void SculptBrush::setReskinTile(voxelutil::ReskinTile tile) {
	_reskinConfig.tile = tile;
	_paramsDirty = true;
}

inline void SculptBrush::setReskinMirrorU(bool mirror) {
	_reskinConfig.mirrorU = mirror;
	_paramsDirty = true;
}

inline void SculptBrush::setReskinMirrorV(bool mirror) {
	_reskinConfig.mirrorV = mirror;
	_paramsDirty = true;
}

inline void SculptBrush::setReskinOffsetU(int offset) {
	_reskinConfig.offsetU = offset;
	_paramsDirty = true;
}

inline void SculptBrush::setReskinOffsetV(int offset) {
	_reskinConfig.offsetV = offset;
	_paramsDirty = true;
}

inline void SculptBrush::setReskinSkinDepth(int depth) {
	_reskinConfig.skinDepth = glm::clamp(depth, 1, MaxReskinDepth);
	_paramsDirty = true;
}

inline void SculptBrush::setReskinZOffset(int offset) {
	_reskinConfig.zOffset = glm::clamp(offset, -MaxReskinDepth, MaxReskinDepth);
	_paramsDirty = true;
}

inline void SculptBrush::setReskinInvertSkin(bool invert) {
	_reskinConfig.invertSkin = invert;
	_paramsDirty = true;
}

inline int SculptBrush::removeAboveDepth() const {
	return _removeAboveDepth;
}

inline void SculptBrush::setRemoveAboveDepth(int depth) {
	_removeAboveDepth = glm::clamp(depth, 0, MaxRemoveAboveDepth);
	_paramsDirty = true;
}

inline bool SculptBrush::extendOnly() const {
	return _extendOnly;
}

inline void SculptBrush::setExtendOnly(bool extend) {
	_extendOnly = extend;
}

inline int SculptBrush::brushRadius() const {
	return _brushRadius;
}

inline void SculptBrush::setBrushRadius(int radius) {
	_brushRadius = glm::clamp(radius, 1, MaxBrushRadius);
}

inline bool SculptBrush::planeFitted() const {
	return _planeFitted;
}

inline const voxel::RawVolume *SculptBrush::skinVolume() const {
	return _skinVolume;
}

inline const core::String &SculptBrush::skinFilePath() const {
	return _skinFilePath;
}

} // namespace voxedit
