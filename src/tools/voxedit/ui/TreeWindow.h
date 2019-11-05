/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"
#include "voxelgenerator/TreeContext.h"

namespace voxedit {

class TreeWindow : public ui::turbobadger::Window {
private:
	using Super = ui::turbobadger::Window;
	voxelgenerator::TreeContext _ctx;

	tb::TBInlineSelect* _trunkHeight;
	tb::TBInlineSelect* _trunkWidth;
	tb::TBInlineSelect* _leavesWidth;
	tb::TBInlineSelect* _leavesHeight;
	tb::TBInlineSelect* _leavesDepth;
public:
	TreeWindow(ui::turbobadger::Window* window, voxelgenerator::TreeType type);

	bool onEvent(const tb::TBWidgetEvent &ev) override;
};

}
