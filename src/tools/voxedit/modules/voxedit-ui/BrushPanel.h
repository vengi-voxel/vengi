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
#include "ui/Panel.h"

namespace command {
struct CommandExecutionListener;
}

namespace voxedit {

class BrushPanel : public ui::Panel {
private:
	using Super = ui::Panel;

	BrushPanelContext _ctx;
	BrushPanelShape _shape;
	BrushPanelStamp _stamp;
	BrushPanelPlane _plane;
	BrushPanelLine _line;
	BrushPanelPaint _paint;
	BrushPanelText _text;
	BrushPanelSelect _select;
	BrushPanelTexture _texture;
	BrushPanelNormal _normal;
	BrushPanelExtrude _extrude;
	BrushPanelTransform _transform;
	BrushPanelSculpt _sculpt;
	BrushPanelRuler _ruler;
	BrushPanelScript _script;

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
	void registerUITests(ImGuiTestEngine *engine, const char *toolbarId, const char *settingsId);
#endif
};

} // namespace voxedit
