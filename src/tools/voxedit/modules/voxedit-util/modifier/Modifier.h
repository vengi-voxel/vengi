/**
 * @file
 */

#pragma once

#include "ModifierButton.h"
#include "ModifierType.h"
#include "SceneModifiedFlags.h"
#include "brush/Brush.h"
#include "brush/BrushType.h"
#include "brush/LineBrush.h"
#include "brush/PaintBrush.h"
#include "brush/PathBrush.h"
#include "brush/PlaneBrush.h"
#include "brush/SelectBrush.h"
#include "brush/ShapeBrush.h"
#include "brush/StampBrush.h"
#include "brush/TextBrush.h"
#include "brush/TextureBrush.h"
#include "core/IComponent.h"
#include "core/collection/Buffer.h"
#include "math/Axis.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/brush/NormalBrush.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Voxel.h"

namespace video {
class Camera;
}

namespace voxedit {

class SceneManager;

/**
 * @brief This class is responsible for manipulating the volume with the configured @c Brush and for
 * doing the selection.
 *
 * There are several modes available. E.g. having the starting point of the aabb on a corner - or
 * at the center, mirroring the modifications and so on.
 */
class Modifier : public core::IComponent {
public:
	using ModifiedRegionCallback =
		std::function<void(const voxel::Region &region, ModifierType type, SceneModifiedFlags flags)>;

protected:
	// lock the modifier to not perform any modification
	// this can be useful when the user is interaction with the ui elements
	// and we don't want to modify the volume
	bool _locked = false;

	/**
	 * timer value which indicates the next execution time in case you keep the
	 * modifier triggered
	 */
	double _nextSingleExecution = 0;
	double _nowSeconds = 0;

	core::Buffer<Brush *> _brushes;
	BrushContext _brushContext;
	BrushType _brushType = BrushType::Shape;
	PlaneBrush _planeBrush;
	ShapeBrush _shapeBrush;
	StampBrush _stampBrush;
	LineBrush _lineBrush;
	PathBrush _pathBrush;
	PaintBrush _paintBrush;
	TextBrush _textBrush;
	SelectBrush _selectBrush;
	TextureBrush _textureBrush;
	NormalBrush _normalBrush;

	ModifierButton _actionExecuteButton;
	ModifierButton _deleteExecuteButton;

public:
	Modifier(SceneManager *sceneMgr);

	/**
	 * @brief Create a Raw Volume Wrapper object while taking the selection into account
	 */
	voxel::RawVolumeWrapper createRawVolumeWrapper(voxel::RawVolume *volume) const;

	void construct() override;
	bool init() override;
	void update(double nowSeconds, const video::Camera *camera);

	/**
	 * Allow to lock the modifier to not perform any modification
	 */
	void lock();
	void unlock();
	inline bool isLocked() const {
		return _locked;
	}

	math::Axis lockedAxis() const;
	void setLockedAxis(math::Axis axis, bool unlock);

	void shutdown() override;

	ModifierType modifierType() const;
	ModifierType setModifierType(ModifierType type);
	// get the mask of supported modifier types for the current active brush
	ModifierType checkModifierType();

	bool isMode(ModifierType modifierType) const;

	const voxel::Voxel &cursorVoxel() const;
	virtual void setCursorVoxel(const voxel::Voxel &voxel);

	/**
	 * @brief Pick the start position of the modifier execution bounding box
	 */
	bool beginBrush();
protected:
	void preExecuteBrush(const voxel::RawVolume *volume);

	/**
	 * @brief Execute the brush operation on the given node volume
	 */
	bool executeBrush(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, ModifierType modifierType,
					  const voxel::Voxel &voxel, const ModifiedRegionCallback &callback = {});
public:
	/**
	 * @brief End the current ModifierType execution and modify the given volume according to the type.
	 * @param[out,in] node The model node to modify
	 * @param callback Called for every region that was modified for the current active modifier.
	 * @note @c start() and @c stop() must be called before and after this method
	 */
	bool execute(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
				 const ModifiedRegionCallback &callback = {});
	void endBrush();
	void abort();
	/**
	 * @brief Actions could get aborted by some external action like hitting esc
	 */
	bool aborted() const;

	BrushType setBrushType(BrushType type);
	BrushType brushType() const;

	const Brush *currentBrush() const;
	Brush *currentBrush();
	const AABBBrush *currentAABBBrush() const;
	AABBBrush *currentAABBBrush();
	voxel::Region calcBrushRegion();

	ShapeBrush &shapeBrush();
	TextBrush &textBrush();
	LineBrush &lineBrush();
	StampBrush &stampBrush();
	PlaneBrush &planeBrush();
	PathBrush &pathBrush();
	PaintBrush &paintBrush();
	BrushContext &brushContext();
	SelectBrush &selectBrush();
	TextureBrush &textureBrush();
	NormalBrush &normalBrush();
	const BrushContext &brushContext() const;

	/**
	 * @sa needsAdditionalAction()
	 */
	void executeAdditionalAction();
	/**
	 * @return @c true if the aabb that was formed has a side that is only 1 voxel
	 * high. This is our indicator for allowing to modify the aabb according to
	 * it's detected axis
	 * @sa executeAdditionalAction()
	 */
	bool needsAdditionalAction();

	/**
	 * @brief Some modifier operate on already existing voxels
	 * @note This has an influence on what the cursor voxel is - either an empty voxel - or an existing one
	 */
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
	void setNormalColorIndex(uint8_t paletteIndex);
	uint8_t normalColorIndex() const;

	voxel::FaceNames cursorFace() const;

	void setGridResolution(int gridSize);
	int gridResolution() const;

	void reset();
};

inline uint8_t Modifier::normalColorIndex() const {
	return _brushContext.normalIndex;
}

inline void Modifier::setNormalColorIndex(uint8_t paletteIndex) {
	_brushContext.normalIndex = paletteIndex;
}

inline math::Axis Modifier::lockedAxis() const {
	return _brushContext.lockedAxis;
}

inline BrushContext &Modifier::brushContext() {
	return _brushContext;
}

inline const BrushContext &Modifier::brushContext() const {
	return _brushContext;
}

inline BrushType Modifier::brushType() const {
	return _brushType;
}

inline TextBrush &Modifier::textBrush() {
	return _textBrush;
}

inline LineBrush &Modifier::lineBrush() {
	return _lineBrush;
}

inline ShapeBrush &Modifier::shapeBrush() {
	return _shapeBrush;
}

inline StampBrush &Modifier::stampBrush() {
	return _stampBrush;
}

inline PlaneBrush &Modifier::planeBrush() {
	return _planeBrush;
}

inline PathBrush &Modifier::pathBrush() {
	return _pathBrush;
}

inline PaintBrush &Modifier::paintBrush() {
	return _paintBrush;
}

inline NormalBrush &Modifier::normalBrush() {
	return _normalBrush;
}

inline SelectBrush &Modifier::selectBrush() {
	return _selectBrush;
}

inline TextureBrush &Modifier::textureBrush() {
	return _textureBrush;
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
	return _brushContext.modifierType;
}

inline bool Modifier::isMode(ModifierType modifierType) const {
	return (_brushContext.modifierType & modifierType) != ModifierType::None;
}

inline bool Modifier::aborted() const {
	if (const AABBBrush *brush = currentAABBBrush()) {
		return brush->aborted(_brushContext);
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

} // namespace voxedit
