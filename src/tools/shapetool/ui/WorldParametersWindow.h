#pragma once

#include "ui/Window.h"
#include "core/Common.h"

using WORLDCTX = voxel::World::WorldContext;
static const ui::Window::Field WORLDFIELDS[] = {
	{INT_FIELD("landscapeoctaves", WORLDCTX, landscapeNoiseOctaves)},
	{FLOAT_FIELD("landscapefrequency", WORLDCTX, landscapeNoiseFrequency)},
	{FLOAT_FIELD("landscapeamplitude", WORLDCTX, landscapeNoiseAmplitude)},
	{FLOAT_FIELD("landscapepersistence", WORLDCTX, landscapeNoisePersistence)},

	{INT_FIELD("mountainoctaves", WORLDCTX, mountainNoiseOctaves)},
	{FLOAT_FIELD("mountainfrequency", WORLDCTX, mountainNoiseFrequency)},
	{FLOAT_FIELD("mountainamplitude", WORLDCTX, mountainNoiseAmplitude)},
	{FLOAT_FIELD("mountainpersistence", WORLDCTX, mountainNoisePersistence)},
};

class WorldParametersWindow: public ui::Window {
private:
	ShapeTool* _tool;
	voxel::World::WorldContext _ctx;
public:
	WorldParametersWindow(ShapeTool* tool) :
			ui::Window(tool), _tool(tool) {
		core_assert(loadResourceFile("ui/window/worldparameters.tb.txt"));
		SetSettings(tb::WINDOW_SETTINGS_TITLEBAR);
		SetOpacity(0.5f);
		fillWidgets(WORLDFIELDS, SDL_arraysize(WORLDFIELDS), &_ctx);
	}

	bool OnEvent(const tb::TBWidgetEvent &ev) override {
		if ((ev.type == tb::EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("reset")) || ev.special_key == tb::TB_KEY_ENTER) {
			fillFields(WORLDFIELDS, SDL_arraysize(WORLDFIELDS), &_ctx);
			_tool->reset(_ctx);
			return true;
		}
		return ui::Window::OnEvent(ev);
	}
};
