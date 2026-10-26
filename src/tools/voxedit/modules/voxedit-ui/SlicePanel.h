/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "core/UUID.h"
#include "ui/Panel.h"
#include "voxel/Region.h"

namespace scenegraph {
class SceneGraphNode;
}

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
	bool _stroke = false;
	core::UUID _strokeNodeUUID;
	voxel::Region _strokeRegion = voxel::Region::InvalidRegion;
	int _strokeView = -1;
	glm::ivec3 _lastPos{0};
	bool _lastPosValid = false;

	static glm::ivec3 voxelPosition(int view, const voxel::Region &region, int slice, int u, int v);
	void drawView(int view, scenegraph::SceneGraphNode &node, const ImVec2 &size);
	void editLine(scenegraph::SceneGraphNode &node, const glm::ivec3 &from, const glm::ivec3 &to, bool erase);
	void finishStroke();

public:
	SlicePanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "slices"), _sceneMgr(sceneMgr) {
	}
	~SlicePanel() override;
	void update(const char *id);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
