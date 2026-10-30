/**
 * @file
 */

#pragma once

#include "app/App.h"
#include "brush/BrushPanelContext.h"
#include "brush/BrushPanelExtrude.h"
#include "brush/BrushPanelLine.h"
#include "brush/BrushPanelNormal.h"
#include "brush/BrushPanelPaint.h"
#include "brush/BrushPanelPlane.h"
#include "brush/BrushPanelRuler.h"
#include "brush/BrushPanelScript.h"
#include "brush/BrushPanelSculpt.h"
#include "brush/BrushPanelSelect.h"
#include "brush/BrushPanelShape.h"
#include "brush/BrushPanelStamp.h"
#include "brush/BrushPanelText.h"
#include "brush/BrushPanelTexture.h"
#include "brush/BrushPanelTransform.h"
#include "core/Function.h"
#include "core/SharedPtr.h"
#include "core/collection/DynamicArray.h"
#include "ui/Panel.h"
#include "voxedit-util/modifier/brush/BrushType.h"

namespace command {
struct CommandExecutionListener;
}

namespace voxedit {

class BrushPanel : public ui::Panel {
private:
	using Super = ui::Panel;

	BrushPanelContext _ctx;
	struct Companion {
		core::String id;
		core::Function<void(BrushPanelContext &, command::CommandExecutionListener &)> update;
		core::Function<void(BrushPanelContext &, command::CommandExecutionListener &)> popups;
	};
	core::DynamicArray<Companion> _companions;

public:
	template<class T>
	core::SharedPtr<T> registerPanel(const core::String &id) {
		auto panel = core::make_shared<T>();
		Companion companion;
		companion.id = id;
		companion.update = [panel](BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
			panel->update(ctx, listener);
		};
		_companions.push_back(core::move(companion));
		return panel;
	}

private:
	void createPopups(command::CommandExecutionListener &listener);
	void addModifiers(command::CommandExecutionListener &listener);
	void brushSettings(command::CommandExecutionListener &listener);

public:
	BrushPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr, const video::TexturePoolPtr &texturePool)
		: Super(app, "brush") {
		_ctx.app = app;
		_ctx.sceneMgr = sceneMgr;
		_ctx.texturePool = texturePool;
	}
	void init();
	float toolbarDockHeight() const;
	void updateToolbar(const char *id, bool sceneMode, command::CommandExecutionListener &listener);
	void updateSettings(const char *id, bool sceneMode, command::CommandExecutionListener &listener);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *toolbarId) override;
#endif
};

} // namespace voxedit
