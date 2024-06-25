/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "ui/Panel.h"
#include "voxelgenerator/TreeContext.h"

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

/**
 * @brief Panel for the tree generator
 */
class TreePanel : public ui::Panel {
private:
	using Super = ui::Panel;

	voxelgenerator::TreeContext _treeGeneratorContext;
	SceneManagerPtr _sceneMgr;

	const char *treeTypeName(int i) const;
	void switchTreeType(voxelgenerator::TreeType treeType);
public:
	TreePanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "tree"), _sceneMgr(sceneMgr) {
	}
	bool init();
	void update(const char *id);
	void shutdown();
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

}
