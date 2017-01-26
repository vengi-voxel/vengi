/**
 * @file
 */

#pragma once

#include "ui/Window.h"

class EditorScene;

namespace voxedit {

class NoiseWindow : public ui::Window {
private:
	using Super = ui::Window;
	EditorScene* _scene;

	tb::TBInlineSelect* _octaves;
	tb::TBEditField* _frequency;
	tb::TBEditField* _persistence;
	tb::TBEditField* _amplitude;
public:
	NoiseWindow(ui::Window* window, EditorScene* scene);

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
};

}
