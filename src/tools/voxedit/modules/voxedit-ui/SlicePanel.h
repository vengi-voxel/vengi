/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "core/UUID.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "math/Axis.h"
#include "palette/PaletteView.h"
#include "ui/IMGUIEx.h"
#include "ui/Panel.h"
#include "video/Camera.h"
#include "voxel/Voxel.h"

namespace scenegraph {
class SceneGraphNode;
}

namespace voxel {
class RawVolume;
class Region;
} // namespace voxel

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

/** Back looks along -Z, Right along -X, Top along -Y. */
enum class SliceView { None = -1, Back = 0, Right, Top, Max };

/** @brief Three editable, axis-aligned slices of the active model in local voxel coordinates. */
class SlicePanel : public ui::Panel {
private:
	using Super = ui::Panel;
	SceneManagerPtr _sceneMgr;
	core::UUID _nodeUUID;
	int _slices[(int)SliceView::Max] = {};
	float _zoom[(int)SliceView::Max] = {1.0f, 1.0f, 1.0f};
	ImVec2 _pan[(int)SliceView::Max] = {};
	bool _background = true;
	bool _grid = true;
	bool _hovered = false;
	bool _gizmoHovered = false;
	video::Camera _cameras[(int)SliceView::Max];
	SliceView _inputView = SliceView::None;
	SliceView _gizmoView = SliceView::None;
	math::Axis _gizmoAxis = math::Axis::X;
	ImVec2 _gizmoStart;
	float _gizmoCell = 1.0f;
	glm::ivec3 _gizmoOffset{0};
	core::SharedPtr<voxel::RawVolume> _gizmoPreview;
	glm::ivec3 _target{0};
	struct TargetView {
		ImDrawList *draw = nullptr;
		ImVec2 origin;
		ImVec2 min;
		ImVec2 max;
		float cell = 1.0f;
	};
	TargetView _targetViews[(int)SliceView::Max];
	core::DynamicArray<voxel::Voxel> _sliceCells;
	core::Buffer<uint8_t> _sliceState;
	ImU32 _fgLut[palette::PaletteMaxColors];
	ImU32 _bgLut[palette::PaletteMaxColors];
	uint64_t _lastPaletteHash = 0;

	static glm::ivec3 voxelPosition(SliceView view, const voxel::Region &region, int slice, int u, int v);
	static glm::ivec2 viewPosition(SliceView view, const voxel::Region &region, const glm::ivec3 &pos);
	void drawTarget(const voxel::Region &region);
	void drawView(SliceView view, scenegraph::SceneGraphNode &node, const ImVec2 &size);
	voxel::Region sliceSelection(const scenegraph::SceneGraphNode &node, SliceView view) const;
	void moveSelection(scenegraph::SceneGraphNode &node, SliceView view, const glm::ivec3 &offset);
	void updateGizmoPreview(scenegraph::SceneGraphNode &node);
	voxel::Voxel displayVoxel(const voxel::RawVolume &volume, const glm::ivec3 &pos) const;
	bool selectionGizmo(SliceView view, scenegraph::SceneGraphNode &node, const ImVec2 &origin, float cell, bool hovered);

public:
	SlicePanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr);
	~SlicePanel() override;
	void update(const char *id);
	bool isHovered() const {
		return _hovered;
	}
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
