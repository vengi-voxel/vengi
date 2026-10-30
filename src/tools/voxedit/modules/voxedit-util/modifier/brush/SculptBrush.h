/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "operation/SculptOperation.h"
#include "app/I18N.h"
#include "core/ArrayLength.h"
#include "core/GLM.h"
#include "core/Optional.h"
#include "core/ScopedPtr.h"
#include "ui/IconsLucide.h"
#include "core/String.h"
#include "palette/Palette.h"
#include "voxel/BitVolume.h"
#include "voxel/DynamicVoxelArray.h"
#include "voxel/SparseVolume.h"
#include "voxel/Face.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeSculpt.h"

#include <climits>
#include <glm/vec3.hpp>

namespace voxel {
class RawVolume;
}

namespace voxedit {

// clang-format off
static constexpr const char *SculptModeStr[] = {
	NC_("Sculpt brush mode", "Erode"),         NC_("Sculpt brush mode", "Grow"),
	NC_("Sculpt brush mode", "Flatten"),       NC_("Sculpt brush mode", "Smooth Additive"),
	NC_("Sculpt brush mode", "Smooth Erode"),  NC_("Sculpt brush mode", "Smooth Gaussian"),
	NC_("Sculpt brush mode", "Smooth Wall"),   NC_("Sculpt brush mode", "Bridge Gap"),
	NC_("Sculpt brush mode", "Squash to Plane"), NC_("Sculpt brush mode", "Extend Plane"),
	NC_("Sculpt brush mode", "Reskin")};
static_assert(lengthof(SculptModeStr) == (int)SculptMode::Max, "SculptModeStr size mismatch");

static constexpr const char *SculptModeIcons[] = {ICON_LC_ERASER,    ICON_LC_SPROUT,     ICON_LC_LAND_PLOT,
                                                  ICON_LC_WAVES,     ICON_LC_WAVES,      ICON_LC_BLEND,
                                                  ICON_LC_BRICK_WALL, ICON_LC_LINK,      ICON_LC_MINIMIZE_2,
                                                  ICON_LC_EXPAND,    ICON_LC_PALETTE};
static_assert(lengthof(SculptModeIcons) == (int)SculptMode::Max, "SculptModeIcons size mismatch");
// clang-format on

/**
 * @brief Sculpts selected voxels by smoothing surface irregularities
 *
 * Captures selected voxels as a snapshot on activation. Each parameter change
 * re-applies the sculpt operation from the original snapshot so changes are
 * absolute and reversible. Adjustable modes stay in a draft until Apply.
 *
 * @ingroup Brushes
 */
class SculptBrush : public Brush, public SculptOperation {
private:
	using Super = Brush;
	core::ScopedPtr<voxel::RawVolume> _ownedSkinVolume;
	const voxel::RawVolume *_skinSourceIdentity = nullptr;
	core::String _skinFilePath;

protected:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	static constexpr int MaxIterations = 250;
	static constexpr int MaxFlattenIterations = 250;
	SculptBrush() : Super(BrushType::Sculpt, ModifierType::Override, ModifierType::Override) {
	}
	SculptBrush(const SculptBrush &other);
	virtual ~SculptBrush() = default;

	bool isSimplePreview() const override {
		return false;
	}

	void acceptParameters(const SculptParameters &parameters) {
		setParameters(parameters);
		_active = false;
	}
	void clearOperation() {
		_snapshotEntries.clear();
		_historyEntries.clear();
		_hasSnapshot = false;
		_cachedBitVolumeValid = false;
		_cachedVoxelMapValid = false;
		_cachedRegionValid = false;
		_paramsDirty = true;
		_active = false;
		_planeFitted = false;
		markDirty();
	}

	void onSceneChange() override;
	void reset() override;
	void onActivated() override;
	void preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) override;
	bool beginBrush(const BrushContext &ctx) override;
	void endBrush(BrushContext &ctx) override;
	bool active() const override;
	voxel::Region calcRegion(const BrushContext &ctx) const override;
	bool managesOwnSelection() const override;
	bool interpolatedStroke() const override {
		return _parameters.mode == SculptMode::ExtendPlane;
	}
	bool executeOnPress() const override {
		return interpolatedStroke();
	}
	bool previewNeedsExistingVolume() const override {
		return true;
	}
	BrushPreviewStyle previewStyle() const override {
		return BrushPreviewStyle::Changes;
	}
	bool wantsContinuousExecution() const override;

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

	// SmoothWall accessors
	static constexpr int MaxSmoothWallClearDepth = 256;
	int smoothWallClearDepth() const;
	void setSmoothWallClearDepth(int depth);
	voxelutil::SmoothWallInterp smoothWallInterp() const;
	void setSmoothWallInterp(voxelutil::SmoothWallInterp interp);
	bool smoothWallFillHoles() const;
	void setSmoothWallFillHoles(bool fillHoles);

	// ExtendPlane accessors
	static constexpr int MaxBrushRadius = 32;
	static constexpr int MaxRemoveAboveDepth = 64;
	int removeAboveDepth() const;
	void setRemoveAboveDepth(int depth);
	bool extendOnly() const;
	void setExtendOnly(bool extend);
	bool removeOnly() const;
	void setRemoveOnly(bool removeOnly);
	int brushRadius() const;
	void setBrushRadius(int radius);
	int previewRadius() const override;
	bool planeFitted() const;

	// Reskin accessors
	static constexpr int MaxReskinDepth = 32;
	static constexpr int MaxReskinRepeat = 64;
	const voxelutil::ReskinConfig &reskinConfig() const;
	void setReskinMode(voxelutil::ReskinMode mode);
	void setReskinFollow(voxelutil::ReskinFollow follow);
	void setReskinRotation(voxelutil::ReskinRotation rotation);
	void setReskinTile(voxelutil::ReskinTile tile);
	void setReskinOffsetU(int offset);
	void setReskinOffsetV(int offset);
	void setReskinSkinDepth(int depth);
	void setReskinZOffset(int offset);
	void setReskinInvertSkin(bool invert);
	void cycleReskinFace();
	void cycleReskinAnchor();
	void setReskinPreview(bool preview);
	void setReskinMaxRepeatU(int count);
	void setReskinMaxRepeatV(int count);
	void setReskinSkinFace(voxel::FaceNames face);

	/**
	 * @brief Set the skin volume for reskin mode.
	 * @param skinVolume The skin volume to apply (not owned, must outlive the brush operation).
	 */
	void setSkinVolume(const voxel::RawVolume *skinVolume);

	/**
	 * @brief Set an owned skin volume loaded from a file. The brush takes ownership.
	 */
	void setOwnedSkinVolume(voxel::RawVolume *skinVolume, const core::String &filePath,
							const palette::Palette *skinPalette = nullptr);

	const voxel::RawVolume *skinVolume() const;
	const core::String &skinFilePath() const;
	const palette::Palette *skinPalette() const;
};

inline bool SculptBrush::managesOwnSelection() const {
	return true;
}

inline bool SculptBrush::wantsContinuousExecution() const {
	return _parameters.mode == SculptMode::ExtendPlane;
}

inline SculptMode SculptBrush::sculptMode() const {
	return _parameters.mode;
}

inline float SculptBrush::strength() const {
	return _parameters.strength;
}

inline void SculptBrush::setStrength(float strength) {
	markSettingsChanged();
	_parameters.strength = glm::clamp(strength, 0.0f, 1.0f);
	_paramsDirty = true;
	markDirty();
}

inline int SculptBrush::iterations() const {
	return _parameters.iterations;
}

inline void SculptBrush::setIterations(int iterations) {
	markSettingsChanged();
	_parameters.iterations = glm::clamp(iterations, 1, MaxFlattenIterations);
	_paramsDirty = true;
	markDirty();
}

inline bool SculptBrush::hasSnapshot() const {
	return _hasSnapshot;
}

inline voxel::FaceNames SculptBrush::flattenFace() const {
	return _parameters.face;
}

inline int SculptBrush::heightThreshold() const {
	return _parameters.heightThreshold;
}

inline void SculptBrush::setHeightThreshold(int threshold) {
	markSettingsChanged();
	_parameters.heightThreshold = glm::clamp(threshold, 1, MaxHeightThreshold);
	_paramsDirty = true;
	markDirty();
}

inline bool SculptBrush::preserveTopHeight() const {
	return _parameters.preserveTopHeight;
}

inline void SculptBrush::setPreserveTopHeight(bool preserve) {
	markSettingsChanged();
	_parameters.preserveTopHeight = preserve;
	_paramsDirty = true;
	markDirty();
}

inline int SculptBrush::trimPerStep() const {
	return _parameters.trimPerStep;
}

inline void SculptBrush::setTrimPerStep(int value) {
	markSettingsChanged();
	_parameters.trimPerStep = glm::clamp(value, 1, MaxTrimPerStep);
	_paramsDirty = true;
	markDirty();
}

inline int SculptBrush::kernelSize() const {
	return _parameters.kernelSize;
}

inline void SculptBrush::setKernelSize(int size) {
	markSettingsChanged();
	_parameters.kernelSize = glm::clamp(size, 1, MaxKernelSize);
	_paramsDirty = true;
	markDirty();
}

inline float SculptBrush::sigma() const {
	return _parameters.sigma;
}

inline void SculptBrush::setSigma(float sigma) {
	markSettingsChanged();
	_parameters.sigma = glm::clamp(sigma, MinSigma, MaxSigma);
	_paramsDirty = true;
	markDirty();
}

inline int SculptBrush::smoothWallClearDepth() const {
	return _parameters.smoothWallClearDepth;
}

inline void SculptBrush::setSmoothWallClearDepth(int depth) {
	markSettingsChanged();
	_parameters.smoothWallClearDepth = glm::clamp(depth, 0, MaxSmoothWallClearDepth);
	_paramsDirty = true;
	markDirty();
}

inline voxelutil::SmoothWallInterp SculptBrush::smoothWallInterp() const {
	return _parameters.smoothWallInterp;
}

inline void SculptBrush::setSmoothWallInterp(voxelutil::SmoothWallInterp interp) {
	markSettingsChanged();
	_parameters.smoothWallInterp = interp;
	_paramsDirty = true;
	markDirty();
}

inline bool SculptBrush::smoothWallFillHoles() const {
	return _parameters.smoothWallFillHoles;
}

inline void SculptBrush::setSmoothWallFillHoles(bool fillHoles) {
	markSettingsChanged();
	_parameters.smoothWallFillHoles = fillHoles;
	_paramsDirty = true;
	markDirty();
}

inline const voxelutil::ReskinConfig &SculptBrush::reskinConfig() const {
	return _parameters.reskin;
}

inline void SculptBrush::setReskinMode(voxelutil::ReskinMode mode) {
	markSettingsChanged();
	_parameters.reskin.mode = mode;
	_paramsDirty = true;
	markDirty();
}

inline void SculptBrush::setReskinFollow(voxelutil::ReskinFollow follow) {
	markSettingsChanged();
	_parameters.reskin.follow = follow;
	_paramsDirty = true;
	markDirty();
}

inline void SculptBrush::setReskinRotation(voxelutil::ReskinRotation rotation) {
	markSettingsChanged();
	_parameters.reskin.rotation = rotation;
	_paramsDirty = true;
	markDirty();
}

inline void SculptBrush::setReskinTile(voxelutil::ReskinTile tile) {
	markSettingsChanged();
	_parameters.reskin.tile = tile;
	_paramsDirty = true;
	markDirty();
}

inline void SculptBrush::setReskinPreview(bool preview) {
	markSettingsChanged();
	_parameters.reskin.preview = preview;
	_paramsDirty = true;
	markDirty();
}

inline void SculptBrush::setReskinMaxRepeatU(int count) {
	markSettingsChanged();
	_parameters.reskin.maxRepeatU = glm::clamp(count, 0, MaxReskinRepeat);
	_paramsDirty = true;
	markDirty();
}

inline void SculptBrush::setReskinMaxRepeatV(int count) {
	markSettingsChanged();
	_parameters.reskin.maxRepeatV = glm::clamp(count, 0, MaxReskinRepeat);
	_paramsDirty = true;
	markDirty();
}

inline void SculptBrush::setReskinSkinFace(voxel::FaceNames face) {
	markSettingsChanged();
	_parameters.reskin.skinFace = face;
	// Re-populate skin depth for the new axis
	if (_parameters.skin) {
		const voxel::Region &sr = _parameters.skin->region();
		const int upIdx = math::getIndexForAxis(voxel::faceToAxis(face));
		const int depthExtent = sr.getUpperCorner()[upIdx] - sr.getLowerCorner()[upIdx] + 1;
		_parameters.reskin.skinDepth = glm::clamp(depthExtent, 1, MaxReskinDepth);
	}
	_paramsDirty = true;
	markDirty();
}

inline void SculptBrush::setReskinOffsetU(int offset) {
	markSettingsChanged();
	_parameters.reskin.offsetU = offset;
	_paramsDirty = true;
	markDirty();
}

inline void SculptBrush::setReskinOffsetV(int offset) {
	markSettingsChanged();
	_parameters.reskin.offsetV = offset;
	_paramsDirty = true;
	markDirty();
}

inline void SculptBrush::setReskinSkinDepth(int depth) {
	markSettingsChanged();
	_parameters.reskin.skinDepth = glm::clamp(depth, 1, MaxReskinDepth);
	_paramsDirty = true;
	markDirty();
}

inline void SculptBrush::setReskinZOffset(int offset) {
	markSettingsChanged();
	_parameters.reskin.zOffset = glm::clamp(offset, -MaxReskinDepth, MaxReskinDepth);
	_paramsDirty = true;
	markDirty();
}

inline void SculptBrush::setReskinInvertSkin(bool invert) {
	markSettingsChanged();
	_parameters.reskin.invertSkin = invert;
	_paramsDirty = true;
	markDirty();
}

inline void SculptBrush::cycleReskinFace() {
	// Cycle through: PositiveX, NegativeX, PositiveY, NegativeY, PositiveZ, NegativeZ
	static constexpr voxel::FaceNames faces[] = {
		voxel::FaceNames::PositiveX, voxel::FaceNames::NegativeX,
		voxel::FaceNames::PositiveY, voxel::FaceNames::NegativeY,
		voxel::FaceNames::PositiveZ, voxel::FaceNames::NegativeZ,
	};
	static constexpr int numFaces = 6;
	for (int i = 0; i < numFaces; ++i) {
		if (faces[i] == _parameters.reskin.skinFace) {
			setReskinSkinFace(faces[(i + 1) % numFaces]);
			return;
		}
	}
	setReskinSkinFace(voxel::FaceNames::PositiveX);
}

inline void SculptBrush::cycleReskinAnchor() {
	int next = ((int)_parameters.reskin.anchor + 1) % (int)voxelutil::ReskinAnchor::Max;
	_parameters.reskin.anchor = (voxelutil::ReskinAnchor)next;
	_paramsDirty = true;
	markDirty();
}

inline int SculptBrush::removeAboveDepth() const {
	return _parameters.removeAboveDepth;
}

inline void SculptBrush::setRemoveAboveDepth(int depth) {
	markSettingsChanged();
	_parameters.removeAboveDepth = glm::clamp(depth, 0, MaxRemoveAboveDepth);
	_paramsDirty = true;
	markDirty();
}

inline bool SculptBrush::extendOnly() const {
	return _parameters.extendOnly;
}

inline void SculptBrush::setExtendOnly(bool extend) {
	markSettingsChanged();
	_parameters.extendOnly = extend;
}

inline bool SculptBrush::removeOnly() const {
	return _parameters.removeOnly;
}

inline void SculptBrush::setRemoveOnly(bool removeOnly) {
	markSettingsChanged();
	_parameters.removeOnly = removeOnly;
}

inline int SculptBrush::brushRadius() const {
	return _parameters.radius;
}

inline int SculptBrush::previewRadius() const {
	return _parameters.radius;
}

inline void SculptBrush::setBrushRadius(int radius) {
	markSettingsChanged();
	_parameters.radius = glm::clamp(radius, 1, MaxBrushRadius);
}

inline bool SculptBrush::planeFitted() const {
	return _planeFitted;
}

inline const voxel::RawVolume *SculptBrush::skinVolume() const {
	return _skinSourceIdentity;
}

inline const core::String &SculptBrush::skinFilePath() const {
	return _skinFilePath;
}

inline const palette::Palette *SculptBrush::skinPalette() const {
	return _parameters.skinPalette.value();
}

} // namespace voxedit
