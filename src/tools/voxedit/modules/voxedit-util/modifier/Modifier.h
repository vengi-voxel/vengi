/**
 * @file
 */

#pragma once

#include "ModifierButton.h"
#include "ModifierType.h"
#include "ModifierVolumeWrapper.h"
#include "Selection.h"
#include "core/ArrayLength.h"
#include "core/GLM.h"
#include "core/IComponent.h"
#include "core/collection/DynamicArray.h"
#include "math/AABB.h"
#include "math/Axis.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxedit-util/modifier/brush/PlaneBrush.h"
#include "voxedit-util/modifier/brush/ScriptBrush.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "voxedit-util/modifier/brush/StampBrush.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Voxel.h"

namespace voxedit {

/**
 * @brief This class is responsible for manipulating the volume with the configured @c Brush and for
 * doing the selection.
 *
 * There are several modes available. E.g. having the starting point of the aabb on a corner - or
 * at the center, mirroring the modifications and so on.
 */
class Modifier : public core::IComponent {
public:
	using Callback = std::function<void(const voxel::Region &region, ModifierType type, bool markUndo)>;

protected:
	Selections _selections;
	bool _selectionValid = false;
	bool _locked = false;
	/**
	 * timer value which indicates the next execution time in case you keep the
	 * modifier triggered
	 */
	double _nextSingleExecution = 0;
	ModifierType _modifierType = ModifierType::Place;

	BrushContext _brushContext;
	BrushType _brushType = BrushType::Shape;
	PlaneBrush _planeBrush;
	ScriptBrush _scriptBrush;
	ShapeBrush _shapeBrush;
	StampBrush _stampBrush;

	ModifierButton _actionExecuteButton;
	ModifierButton _deleteExecuteButton;

	bool lineModifier(voxel::RawVolume *volume, const Callback &callback);
	bool pathModifier(voxel::RawVolume *volume, const Callback &callback);

	bool runModifier(
		scenegraph::SceneGraph &sceneGraph, voxel::RawVolume *volume, ModifierType modifierType,
		const voxel::Voxel &voxel, const Callback &callback = [](const voxel::Region &, ModifierType, bool) {});

	Brush *activeBrush();
public:
	Modifier();

	/**
	 * @brief Create a Raw Volume Wrapper object while taking the selection into account
	 */
	voxel::RawVolumeWrapper createRawVolumeWrapper(voxel::RawVolume *volume) const;

	void construct() override;
	bool init() override;
	void update(double nowSeconds);

	/**
	 * Allow to lock the modifier to not perform any modification
	 */
	void lock();
	void unlock();
	inline bool isLocked() const {
		return _locked;
	}

	void shutdown() override;

	virtual bool select(const glm::ivec3 &mins, const glm::ivec3 &maxs);
	virtual void unselect();
	virtual void invert(const voxel::Region &region);
	const Selections &selections() const;

	ModifierType modifierType() const;
	void setModifierType(ModifierType type);

	bool isMode(ModifierType modifierType) const;

	const voxel::Voxel &cursorVoxel() const;
	virtual void setCursorVoxel(const voxel::Voxel &voxel);

	/**
	 * @brief Pick the start position of the modifier execution bounding box
	 */
	bool start();
	/**
	 * @brief End the current ModifierType execution and modify the given volume according to the type.
	 * @param[out,in] volume The volume to modify
	 * @param callback Called for every region that was modified for the current active modifier.
	 * @note @c start() and @c stop() must be called before and after this method
	 */
	bool execute(scenegraph::SceneGraph &sceneGraph, voxel::RawVolume *volume, const Callback &callback);
	void stop();
	/**
	 * @brief Actions could get aborted by some external action like hitting esc
	 */
	bool aborted() const;

	void setBrushType(BrushType type);
	BrushType brushType() const;

	const ShapeBrush *activeShapeBrush() const;
	ShapeBrush *activeShapeBrush();
	glm::ivec3 calcShapeBrushRegionSize();
	voxel::Region calcBrushRegion();

	ScriptBrush &scriptBrush();
	ShapeBrush &shapeBrush();
	StampBrush &stampBrush();
	BrushContext &brushContext();

	/**
	 * @sa needsFurtherAction()
	 */
	void executeAdditionalAction();
	/**
	 * @return @c true if the aabb that was formed has a side that is only 1 voxel
	 * high. This is our indicator for allowing to modify the aabb according to
	 * it's detected axis
	 * @sa executeAdditionalAction()
	 */
	bool needsFurtherAction();

	bool modifierTypeRequiresExistingVoxel() const;

	/**
	 * @param[in] pos The position inside the volume - given in absolute world coordinates
	 * @param[in] face The voxel::FaceNames values where the trace hits an existing voxel
	 */
	void setCursorPosition(const glm::ivec3 &pos, voxel::FaceNames face);
	const glm::ivec3 &cursorPosition() const;
	glm::ivec3 currentCursorPosition();

	const glm::ivec3 &referencePosition() const;
	virtual void setReferencePosition(const glm::ivec3 &pos);

	const voxel::Voxel &hitCursorVoxel() const;
	void setHitCursorVoxel(const voxel::Voxel &);
	void setVoxelAtCursor(const voxel::Voxel &voxel);

	voxel::FaceNames cursorFace() const;

	void setGridResolution(int gridSize);
	int gridResolution() const;

	void reset();
};

inline BrushContext &Modifier::brushContext() {
	return _brushContext;
}

inline BrushType Modifier::brushType() const {
	return _brushType;
}

inline ShapeBrush &Modifier::shapeBrush() {
	return _shapeBrush;
}

inline StampBrush &Modifier::stampBrush() {
	return _stampBrush;
}

inline ScriptBrush &Modifier::scriptBrush() {
	return _scriptBrush;
}

inline int Modifier::gridResolution() const {
	return _brushContext.gridResolution;
}

inline const voxel::Voxel &Modifier::hitCursorVoxel() const {
	return _brushContext.hitCursorVoxel;
}

inline const glm::ivec3 &Modifier::referencePosition() const {
	return _brushContext.referencePos;
}

inline ModifierType Modifier::modifierType() const {
	return _modifierType;
}

inline bool Modifier::isMode(ModifierType modifierType) const {
	return (_modifierType & modifierType) == modifierType;
}

inline bool Modifier::aborted() const {
	if (const ShapeBrush *brush = activeShapeBrush()) {
		return brush->aborted();
	}
	return false;
}

inline void Modifier::setCursorPosition(const glm::ivec3 &pos, voxel::FaceNames face) {
	_brushContext.cursorPosition = pos;
	_brushContext.cursorFace = face;
}

inline voxel::FaceNames Modifier::cursorFace() const {
	return _brushContext.cursorFace;
}

inline void Modifier::setCursorVoxel(const voxel::Voxel &voxel) {
	if (voxel::isAir(voxel.getMaterial())) {
		return;
	}
	_brushContext.cursorVoxel = voxel;
}

inline const voxel::Voxel &Modifier::cursorVoxel() const {
	return _brushContext.cursorVoxel;
}

inline const glm::ivec3 &Modifier::cursorPosition() const {
	return _brushContext.cursorPosition;
}

inline const Selections &Modifier::selections() const {
	return _selections;
}

} // namespace voxedit
