/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "core/UUID.h"
#include "math/Axis.h"
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

/** @brief Three editable, axis-aligned slices of the active model in local voxel coordinates. */
class SlicePanel : public ui::Panel {
private:
	using Super = ui::Panel;
	SceneManagerPtr _sceneMgr;
	core::UUID _nodeUUID;
	int _slices[3] = {};
	float _zoom[3] = {1.0f, 1.0f, 1.0f};
	ImVec2 _pan[3] = {};
	bool _background = true;
	bool _grid = true;
	bool _hovered = false;
	bool _gizmoHovered = false;
	int _inputView = -1;
	video::Camera _cameras[3];
	int _gizmoView = -1;
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
	TargetView _targetViews[3];

	static glm::ivec3 voxelPosition(int view, const voxel::Region &region, int slice, int u, int v);
	static glm::ivec2 viewPosition(int view, const voxel::Region &region, const glm::ivec3 &pos);
	void drawTarget(const voxel::Region &region);
	void drawView(int view, scenegraph::SceneGraphNode &node, const ImVec2 &size);
	voxel::Region sliceSelection(const scenegraph::SceneGraphNode &node, int view) const;
	void moveSelection(scenegraph::SceneGraphNode &node, int view, const glm::ivec3 &offset);
	void updateGizmoPreview(scenegraph::SceneGraphNode &node);
	voxel::Voxel displayVoxel(const voxel::RawVolume &volume, const glm::ivec3 &pos) const;
	bool selectionGizmo(int view, scenegraph::SceneGraphNode &node, const ImVec2 &origin, float cell, bool hovered);

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
