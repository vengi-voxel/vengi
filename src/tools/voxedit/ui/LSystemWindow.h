/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"

class EditorScene;

namespace voxedit {

class LSystemWindow: public ui::turbobadger::Window {
private:
	using Super = ui::turbobadger::Window;
	EditorScene* _scene;
	tb::TBEditField* _axiom = nullptr;
	tb::TBInlineSelect* _generations = nullptr;
	tb::TBSelectList* _productionRules = nullptr;

	void save(const std::string& file);
	void load(const std::string& file);
public:
	LSystemWindow(ui::turbobadger::Window* window, EditorScene* scene);
	~LSystemWindow();

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
};

}
