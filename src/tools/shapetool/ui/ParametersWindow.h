#pragma once

#include "ui/Window.h"
#include "core/Common.h"

using CTX = voxel::World::WorldContext;
static const ui::Window::Field FIELDS[] = {
	{INT_FIELD("landscapeoctaves", CTX, landscapeNoiseOctaves)},
	{FLOAT_FIELD("landscapefrequency", CTX, landscapeNoiseFrequency)},
	{FLOAT_FIELD("landscapeamplitude", CTX, landscapeNoiseAmplitude)},
	{FLOAT_FIELD("landscapepersistence", CTX, landscapeNoisePersistence)},

	{INT_FIELD("mountainoctaves", CTX, mountainNoiseOctaves)},
	{FLOAT_FIELD("mountainfrequency", CTX, mountainNoiseFrequency)},
	{FLOAT_FIELD("mountainamplitude", CTX, mountainNoiseAmplitude)},
	{FLOAT_FIELD("mountainpersistence", CTX, mountainNoisePersistence)},
};

class ParametersWindow: public ui::Window {
private:
	ShapeTool* _tool;
	voxel::World::WorldContext _ctx;
public:
	ParametersWindow(ShapeTool* tool) :
			ui::Window(tool), _tool(tool) {
		core_assert(loadResourceFile("ui/window/parameters.tb.txt"));
		SetSettings(tb::WINDOW_SETTINGS_TITLEBAR);
		SetOpacity(0.5f);
		fillWidgets(FIELDS, SDL_arraysize(FIELDS), &_ctx);
	}

	bool OnEvent(const tb::TBWidgetEvent &ev) override {
		if ((ev.type == tb::EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("reset")) || ev.special_key == tb::TB_KEY_ENTER) {
			fillFields(FIELDS, SDL_arraysize(FIELDS), &_ctx);
			_tool->reset(_ctx);
			return true;
		}
		return ui::Window::OnEvent(ev);
	}
};
