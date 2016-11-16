/**
 * @file
 */

#pragma once

#include "ui/Window.h"

class EditorScene;

namespace voxedit {

class LSystemWindow: public ui::Window {
private:
	using Super = ui::Window;
	EditorScene* _scene;
	tb::TBEditField* _axiom = nullptr;
	tb::TBInlineSelect* _generations = nullptr;
	tb::TBSelectList* _productionRules = nullptr;
public:
	LSystemWindow(ui::Window* window, EditorScene* scene);

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
};

}
