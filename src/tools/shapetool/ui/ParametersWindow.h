#pragma once

#include "ui/Window.h"
#include "core/Common.h"

class ParametersWindow: public ui::Window {
private:
	ShapeTool* _tool;
	voxel::World::WorldContext _ctx;
public:
	ParametersWindow(ShapeTool* tool) :
			ui::Window(tool), _tool(tool) {
		core_assert(loadResourceFile("ui/window/parameters.tb.txt"));
		SetSettings(tb::WINDOW_SETTINGS_TITLEBAR);
	}

	bool OnEvent(const tb::TBWidgetEvent &ev) override {
		if ((ev.type == tb::EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("reset")) || ev.special_key == tb::TB_KEY_ENTER) {
			using CTX = voxel::World::WorldContext;
			static const Field FIELDS[] = {
				{INT_FIELD("landscapeoctaves", CTX, landscapeNoiseOctaves)},
				{INT_FIELD("landscapefrequency", CTX, landscapeNoiseFrequency)},
				{INT_FIELD("landscapeamplitude", CTX, landscapeNoiseAmplitude)},
				{INT_FIELD("landscapepersistence", CTX, landscapeNoisePersistence)},
			};

			fillFields(FIELDS, SDL_arraysize(FIELDS), &_ctx);
			_tool->reset(_ctx);
			return true;
		}
		return ui::Window::OnEvent(ev);
	}
};
