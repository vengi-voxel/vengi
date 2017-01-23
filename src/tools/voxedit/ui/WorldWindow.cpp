/**
 * @file
 */

#include "WorldWindow.h"
#include "core/Common.h"
#include "editorscene/EditorScene.h"

namespace voxedit {

using WORLDCTX = voxel::WorldContext;
static const ui::Window::Field WORLDFIELDS[] = {
	{INT_FIELD("landscapeoctaves", WORLDCTX, landscapeNoiseOctaves)},
	{FLOAT_FIELD("landscapefrequency", WORLDCTX, landscapeNoiseFrequency)},
	{FLOAT_FIELD("landscapeamplitude", WORLDCTX, landscapeNoiseAmplitude)},
	{FLOAT_FIELD("landscapepersistence", WORLDCTX, landscapeNoisePersistence)},

	{INT_FIELD("mountainoctaves", WORLDCTX, mountainNoiseOctaves)},
	{FLOAT_FIELD("mountainfrequency", WORLDCTX, mountainNoiseFrequency)},
	{FLOAT_FIELD("mountainamplitude", WORLDCTX, mountainNoiseAmplitude)},
	{FLOAT_FIELD("mountainpersistence", WORLDCTX, mountainNoisePersistence)},

	{INT_FIELD("caveoctaves", WORLDCTX, caveNoiseOctaves)},
	{FLOAT_FIELD("cavefrequency", WORLDCTX, caveNoiseFrequency)},
	{FLOAT_FIELD("caveamplitude", WORLDCTX, caveNoiseAmplitude)},
	{FLOAT_FIELD("cavepersistence", WORLDCTX, caveNoisePersistence)},
	{FLOAT_FIELD("cavedensitythreshold", WORLDCTX, caveDensityThreshold)}
};

WorldWindow::WorldWindow(ui::Window* window, EditorScene* scene, const io::FilePtr& luaFile) :
			ui::Window(window), _scene(scene) {
	core_assert_always(loadResourceFile("ui/window/voxedit-world.tb.txt"));
	SetSettings(tb::WINDOW_SETTINGS_TITLEBAR);
	SetOpacity(0.8f);
	if (!_ctx.load(luaFile)) {
		Log::warn("Could not load the world context from the lua file");
	}
	fillWidgets(WORLDFIELDS, SDL_arraysize(WORLDFIELDS), &_ctx);
}

bool WorldWindow::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (ev.target->GetID() == TBIDC("ok")) {
			fillFields(WORLDFIELDS, SDL_arraysize(WORLDFIELDS), &_ctx);
			_scene->world(_ctx);
			Close();
			return true;
		} else if (ev.target->GetID() == TBIDC("cancel")) {
			Close();
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_KEY_DOWN) {
		if (ev.special_key == tb::TB_KEY_ESC) {
			Close();
			return true;
		}
	}
	return ui::Window::OnEvent(ev);
}

}
