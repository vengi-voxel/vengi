/**
 * @file
 * @defgroup Brushes Brushes
 * @{
 * Editor brushes.
 * @}
 */

#pragma once

#include "BrushType.h"
#include "core/DirtyState.h"
#include "core/IComponent.h"
#include "core/String.h"
#include "math/Axis.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/SceneModifiedFlags.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
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

	// brushes that e.g. span an aabb behave differently if the view is fixed and in ortho mode. As you don't have the
	// chance to really span the aabb by giving the mins and maxs.
	bool fixedOrthoSideView = false;
	int gridResolution = 1;

	// used for clamping the brush region to the volume region
	voxel::Region targetVolumeRegion;

	// the position of the cursor before any clamping was applied or the brush was executed
	glm::ivec3 prevCursorPosition{0};

	ModifierType modifierType = ModifierType::Place;
};

/**
 * @brief Base class for all brushes
 * @ingroup Brushes
 * @sa ModifierVolumeWrapper
 */
class Brush : public core::IComponent, public core::DirtyState {
protected:
	const BrushType _brushType;
	const ModifierType _defaultModifier;
	const ModifierType _supportedModifiers;
	SceneModifiedFlags _sceneModifiedFlags = SceneModifiedFlags::All;

	glm::ivec3 _referencePosition{0};

	core::String _errorReason;

	/**
	 * The mirror position is based on the reference position whenever the mirror axis is set
	 */
	glm::ivec3 _mirrorPos{0};
	math::Axis _mirrorAxis = math::Axis::None;
	void toggleMirrorAxis(math::Axis axis, const glm::ivec3 &mirrorPos);

	// change the cursor position if the brush region is outside the volume
	bool _brushClamping = false;

	Brush(BrushType brushType, ModifierType defaultModifier = ModifierType::Place,
		  ModifierType supportedModifiers = (ModifierType::Place | ModifierType::Erase | ModifierType::Override))
		: _brushType(brushType), _defaultModifier(defaultModifier), _supportedModifiers(supportedModifiers) {
	}

	/**
	 * @brief Generate the voxels here
	 *
	 * @sa execute() - this method is called by execute() but if mirroring is supported, the regions might differ for
	 * each call
	 *
	 * @param[in] sceneGraph The scene graph to operate on
	 * @param[in] wrapper The volume wrapper to operate on
	 * @param[in] ctx The brush context
	 * @param[in] region The region might be invalid and depends on the calcRegion() implementation of the brush
	 *
	 * @note In case of mirroring the region might be adjusted
	 */
	virtual void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						  const voxel::Region &region) = 0;

	// this error text can be shown in the ui if a brush is e.g. not properly initialized or configured
	void setErrorReason(const core::String &reason) {
		_errorReason = reason;
	}

public:
	void markDirty() override {
		core::DirtyState::markDirty();
	}

	const core::String &errorReason() const {
		return _errorReason;
	}

	/**
	 * @brief Start the brush action
	 * @sa preExecute()
	 * @sa execute()
	 * @sa endBrush()
	 */
	virtual bool beginBrush(const BrushContext &ctx);

	/**
	 * @brief Prepare the brush state
	 * @param[in] ctx The brush context
	 * @param[in] volume The volume that is currently active - can be null
	 * @sa beginBrush()
	 * @sa execute()
	 * @sa endBrush()
	 */
	virtual void preExecute(const BrushContext &ctx, const voxel::RawVolume *volume);

	/**
	 * @brief Execute the brush action on the given volume and also handles mirroring
	 * @param[in] sceneGraph The scene graph to operate on
	 * @param[in] wrapper The volume wrapper to operate on
	 * @param[in] ctx The brush context
	 * @return @c true if the brush action was successful
	 * @sa beginBrush()
	 * @sa preExecute()
	 * @sa endBrush()
	 */
	virtual bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx);

	/**
	 * @brief Called when brush action was executed
	 * @sa beginBrush()
	 * @sa preExecute()
	 * @sa execute()
	 */
	virtual void endBrush(BrushContext &ctx);

	/**
	 * @brief Abort the brush lifecycle
	 * @sa beginBrush()
	 * @sa execute()
	 */
	virtual void abort(BrushContext &ctx) {
	}

	/**
	 * @brief Reset the brush state and force a re-creating of the preview volume
	 */
	virtual void reset();
	/**
	 * @brief E.g. checks whether the brush preview volume needs to be updated
	 * @sa markDirty()
	 */
	virtual void update(const BrushContext &ctx, double nowSeconds);
	/**
	 * @return the type of the brush as string
	 * @sa type()
	 */
	core::String name() const;
	/**
	 * @sa name()
	 */
	BrushType type() const;

	SceneModifiedFlags sceneModifiedFlags() const;

	/**
	 * @brief The region that this brush is modifying (without mirroring or anything like that)
	 */
	virtual voxel::Region calcRegion(const BrushContext &ctx) const = 0;

	/**
	 * @brief allow to change the modifier type if the brush doesn't support the given mode
	 * @return the supported modifier type
	 */
	ModifierType modifierType(ModifierType type = ModifierType::None) const;

	void setBrushClamping(bool brushClamping);
	bool brushClamping() const;

	/**
	 * @brief Determine whether the brush should get rendered
	 */
	virtual bool active() const;

	void construct() override;
	bool init() override;
	void shutdown() override;

	bool getMirrorAABB(glm::ivec3 &mins, glm::ivec3 &maxs) const;
	bool setMirrorAxis(math::Axis axis, const glm::ivec3 &mirrorPos);
	math::Axis mirrorAxis() const;
	const glm::ivec3 &mirrorPos() const;
};

inline const glm::ivec3 &Brush::mirrorPos() const {
	return _mirrorPos;
}

inline math::Axis Brush::mirrorAxis() const {
	return _mirrorAxis;
}

inline BrushType Brush::type() const {
	return _brushType;
}

inline core::String Brush::name() const {
	return BrushTypeStr[(int)_brushType];
}

} // namespace voxedit
