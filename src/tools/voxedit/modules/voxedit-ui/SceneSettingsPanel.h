/**
 * @file
 */

#pragma once

#include "app/App.h"
#include "command/CommandHandler.h"
#include "core/SharedPtr.h"
#include "ui/Panel.h"

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

enum class ShadingMode : uint8_t {
	Unlit = 0,	// Pure voxel colors, no lighting
	Lit = 1,	// Ambient + diffuse lighting, no shadows
	Shadows = 2 // Full lighting with shadows
};

class SceneSettingsPanel : public ui::Panel {
private:
	using Super = ui::Panel;
	SceneManagerPtr _sceneMgr;
	core::VarPtr _rendershadow;
	core::VarPtr _ambientColor;
	core::VarPtr _diffuseColor;
	core::VarPtr _sunAngle;
	core::VarPtr _shadingMode;
	void sceneColors(ShadingMode shadingMode);
	void sceneShadowAndSun(ShadingMode shadingMode);

public:
	SceneSettingsPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr);
	void init();
	void update(const char *id, command::CommandExecutionListener &listener);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
