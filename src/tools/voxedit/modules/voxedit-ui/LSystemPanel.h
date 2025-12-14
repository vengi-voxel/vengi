/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "ui/Panel.h"
#include "voxedit-util/SceneManager.h"
#include "voxelgenerator/LSystem.h"

namespace voxedit {

class LSystemPanel : public ui::Panel {
private:
	using Super = ui ::Panel;
	core::DynamicArray<voxelgenerator::lsystem::LSystemTemplate> _templates;
	voxelgenerator::lsystem::LSystemConfig _conf;
	SceneManagerPtr _sceneMgr;
	int _templateIdx = -1;

	void copyRulesToClipboard();
	void pasteRulesFromClipboard();

public:
	LSystemPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "lsystem"), _sceneMgr(sceneMgr) {
	}
	bool init();
	void update(const char *id);
	void shutdown();
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
