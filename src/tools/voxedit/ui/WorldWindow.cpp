/**
 * @file
 */

#include "WorldWindow.h"
#include "core/Common.h"
#include "core/Array.h"
#include "editorscene/EditorScene.h"

namespace voxedit {

using WORLDCTX = voxel::WorldContext;
static const ui::turbobadger::Window::Field WORLDFIELDS[] = {
	{INT_FIELD("landscapeoctaves", WORLDCTX, landscapeNoiseOctaves)},
	{FLOAT_FIELD("landscapefrequency", WORLDCTX, landscapeNoiseFrequency)},
	{FLOAT_FIELD("landscapegain", WORLDCTX, landscapeNoiseGain)},
	{FLOAT_FIELD("landscapelacunarity", WORLDCTX, landscapeNoiseLacunarity)},

	{INT_FIELD("mountainoctaves", WORLDCTX, mountainNoiseOctaves)},
	{FLOAT_FIELD("mountainfrequency", WORLDCTX, mountainNoiseFrequency)},
	{FLOAT_FIELD("mountaingain", WORLDCTX, mountainNoiseGain)},
	{FLOAT_FIELD("mountainlacunarity", WORLDCTX, mountainNoiseLacunarity)},

	{INT_FIELD("caveoctaves", WORLDCTX, caveNoiseOctaves)},
	{FLOAT_FIELD("cavefrequency", WORLDCTX, caveNoiseFrequency)},
	{FLOAT_FIELD("cavegain", WORLDCTX, caveNoiseGain)},
	{FLOAT_FIELD("cavelacunarity", WORLDCTX, caveNoiseLacunarity)},
	{FLOAT_FIELD("cavedensitythreshold", WORLDCTX, caveDensityThreshold)}
};

WorldWindow::WorldWindow(ui::turbobadger::Window* window, EditorScene* scene, const std::string& luaString) :
			Super(window), _scene(scene) {
	core_assert_always(loadResourceFile("ui/window/voxedit-world.tb.txt"));
	SetSettings(tb::WINDOW_SETTINGS_TITLEBAR);
	SetOpacity(0.8f);
	if (!_ctx.load(luaString)) {
		Log::warn("Could not load the world context from the lua file");
	}
	fillWidgets(WORLDFIELDS, lengthof(WORLDFIELDS), &_ctx);
}

bool WorldWindow::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (ev.target->GetID() == TBIDC("ok")) {
			fillFields(WORLDFIELDS, lengthof(WORLDFIELDS), &_ctx);
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
	return Super::OnEvent(ev);
}

}
