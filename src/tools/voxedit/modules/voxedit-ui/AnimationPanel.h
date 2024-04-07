/**
 * @file
 */

#pragma once

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
	core::String _newAnimation;
	SceneManagerPtr _sceneMgr;

public:
	AnimationPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "animationpanel"), _sceneMgr(sceneMgr) {
	}
	void update(const char *title, command::CommandExecutionListener &listener, AnimationTimeline *animationTimeline);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *title) override;
#endif
};

} // namespace voxedit
