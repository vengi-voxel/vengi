/**
 * @file
 */

#pragma once

#include "app/App.h"
#include "core/SharedPtr.h"
#include "ui/Panel.h"
#include "core/String.h"

namespace command {
struct CommandExecutionListener;
}

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;
class AnimationTimeline;

/**
 * @brief The animation panel will open all available animations for a model and allows you to switch
 * the animation.
 */
class AnimationPanel : public ui::Panel {
private:
	using Super = ui ::Panel;
	SceneManagerPtr _sceneMgr;
	core::VarPtr _popupCreateAnimation;
	core::String _newAnimation;
	core::String _selectedAnimation;
	bool _copyExistingAnimation = false;

	void popupCreateAnimation();

public:
	AnimationPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr)
		: Super(app, "animationpanel"), _sceneMgr(sceneMgr) {
	}
	void update(const char *id, command::CommandExecutionListener &listener, AnimationTimeline *animationTimeline);
	void registerPopups();
	bool init();
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
