/**
 * @file
 */

#pragma once

#include "AABBBrush.h"
#include "app/I18NMarkers.h"
#include "core/ArrayLength.h"
#include "select/All.h"
#include "select/Box3D.h"
#include "select/Circle.h"
#include "select/Connected.h"
#include "select/FlatSurface.h"
#include "select/FuzzyColor.h"
#include "select/Lasso.h"
#include "select/Paint.h"
#include "select/SameColor.h"
#include "select/Script.h"
#include "select/Surface.h"
#include "ui/IconsLucide.h"
#include "voxel/Region.h"
#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace voxel {
class RawVolume;
} // namespace voxel

namespace voxedit {

class SceneManager;
class LUASelectionMode;

/**
 * @brief Selection mode for the SelectBrush
 */
enum class SelectMode : uint8_t {
	All,
	/** Select only visible surface voxels in the AABB region */
	Surface,
	/** Select only voxels with the same color as the clicked voxel */
	SameColor,
	/** Select only voxels with the similar color as the clicked voxel */
	FuzzyColor,
	/** Select voxels connected to the clicked voxel with the same color (flood fill) */
	Connected,
	/** Flood-fill select all connected solid voxels that share the same exposed face as the clicked voxel */
	FlatSurface,
	/** Replace the current selection with all solid voxels in the drawn 3D box region */
	Box3D,
	/** Select surface voxels within a circular radius from the clicked center point */
	Circle,
	/** Free-form polygon selection: click vertices to build a polygon, close it to select enclosed surface voxels */
	Lasso,
	/** Continuous paint-style selection: hold mouse and drag to select solid voxels within brush radius.
	 *  Uses single mode for continuous execution. Single undo entry on release. */
	Paint,
	/** Selection mode driven by a user-supplied Lua script */
	Script,
	Max
};

// clang-format off
static constexpr const char *SelectModeStr[] = {
	NC_("SelectMode", "All"),          NC_("SelectMode", "Surface"),      NC_("SelectMode", "Same Color"),
	NC_("SelectMode", "Fuzzy Color"),  NC_("SelectMode", "Connected"),    NC_("SelectMode", "Flat Surface"),
	NC_("SelectMode", "3D Box"),       NC_("SelectMode", "Circle"),
	NC_("SelectMode", "Lasso"),        NC_("SelectMode", "Paint"),
	NC_("SelectMode", "Script")};
static_assert(lengthof(SelectModeStr) == (int)SelectMode::Max, "SelectModeStr size mismatch");

static constexpr const char *SelectModeIcons[] = {
	ICON_LC_SQUARE_DASHED,       // All
	ICON_LC_SCAN,                // Surface
	ICON_LC_PIPETTE,             // SameColor
	ICON_LC_WAND_SPARKLES,       // FuzzyColor
	ICON_LC_WAYPOINTS,           // Connected
	ICON_LC_LAND_PLOT,           // FlatSurface
	ICON_LC_BOX,                 // Box3D
	ICON_LC_CIRCLE,              // Circle
	ICON_LC_LASSO,               // Lasso
	ICON_LC_PAINTBRUSH,          // Paint
	ICON_LC_CODE,                // Script
};
static_assert(lengthof(SelectModeIcons) == (int)SelectMode::Max, "SelectModeIcons size mismatch");
// clang-format on

/**
 * @ingroup Brushes
 */
class SelectBrush : public AABBBrush {
private:
	using Super = AABBBrush;
	SceneManager *_sceneManager = nullptr;
	SelectMode _selectMode = SelectMode::All;
	int _luaSelectionModeIndex = -1;

	select::Circle _circleStrategy;
	select::Lasso _lassoStrategy;
	select::Paint _paintStrategy;
	select::Box3D _box3DStrategy;
	select::FuzzyColor _fuzzyColorStrategy;
	select::FlatSurface _flatSurfaceStrategy;
	select::Script _scriptStrategy;

	select::All _selectAll;
	select::Surface _selectSurface;
	select::SameColor _selectSameColor;
	select::Connected _selectConnected;

	select::Strategy *_strategies[(int)SelectMode::Max];

	select::Strategy *activeStrategy() const;
	select::AABBBrushState buildState(const BrushContext &ctx) const;

	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	SelectBrush(SceneManager *sceneManager);
	virtual ~SelectBrush() = default;

	voxel::Region calcRegion(const BrushContext &ctx) const override;
	bool managesOwnSelection() const override;
	bool active() const override;
	void update(const BrushContext &ctx, double nowSeconds) override;
	bool needsAdditionalAction(const BrushContext &ctx) const override;
	bool beginBrush(const BrushContext &ctx) override;
	void reset() override;
	void onSceneChange() override;

	void setSelectMode(SelectMode mode);
	SelectMode selectMode() const;

	void setLuaSelectionMode(int index, LUASelectionMode *mode);
	int luaSelectionModeIndex() const;
	LUASelectionMode *activeLuaSelectionMode() const;

	void endBrush(BrushContext &ctx) override;
	bool hasPendingChanges() const override;
	voxel::Region revertChanges(voxel::RawVolume *volume) override;
	voxel::Region consumePendingUndoRegion() override;
	void abort(BrushContext &ctx) override;

	select::Circle &circle();
	const select::Circle &circle() const;
	select::Lasso &lasso();
	const select::Lasso &lasso() const;
	select::Paint &paint();
	const select::Paint &paint() const;
	select::Box3D &box3D();
	const select::Box3D &box3D() const;
	select::FuzzyColor &fuzzyColor();
	const select::FuzzyColor &fuzzyColor() const;
	select::FlatSurface &flatSurface();
	const select::FlatSurface &flatSurface() const;
	select::Script &script();
	const select::Script &script() const;
};

inline bool SelectBrush::managesOwnSelection() const {
	return true;
}

inline SelectMode SelectBrush::selectMode() const {
	return _selectMode;
}

inline int SelectBrush::luaSelectionModeIndex() const {
	return _luaSelectionModeIndex;
}

inline LUASelectionMode *SelectBrush::activeLuaSelectionMode() const {
	return _scriptStrategy.activeLuaMode();
}

inline select::Strategy *SelectBrush::activeStrategy() const {
	return _strategies[(int)_selectMode];
}

inline select::Circle &SelectBrush::circle() {
	return _circleStrategy;
}

inline const select::Circle &SelectBrush::circle() const {
	return _circleStrategy;
}

inline select::Lasso &SelectBrush::lasso() {
	return _lassoStrategy;
}

inline const select::Lasso &SelectBrush::lasso() const {
	return _lassoStrategy;
}

inline select::Paint &SelectBrush::paint() {
	return _paintStrategy;
}

inline const select::Paint &SelectBrush::paint() const {
	return _paintStrategy;
}

inline select::Box3D &SelectBrush::box3D() {
	return _box3DStrategy;
}

inline const select::Box3D &SelectBrush::box3D() const {
	return _box3DStrategy;
}

inline select::FuzzyColor &SelectBrush::fuzzyColor() {
	return _fuzzyColorStrategy;
}

inline const select::FuzzyColor &SelectBrush::fuzzyColor() const {
	return _fuzzyColorStrategy;
}

inline select::FlatSurface &SelectBrush::flatSurface() {
	return _flatSurfaceStrategy;
}

inline const select::FlatSurface &SelectBrush::flatSurface() const {
	return _flatSurfaceStrategy;
}

inline select::Script &SelectBrush::script() {
	return _scriptStrategy;
}

inline const select::Script &SelectBrush::script() const {
	return _scriptStrategy;
}

} // namespace voxedit
