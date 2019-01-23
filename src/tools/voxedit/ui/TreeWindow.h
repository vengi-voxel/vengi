/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"
#include "voxel/TreeContext.h"

namespace voxedit {

class TreeWindow : public ui::turbobadger::Window {
private:
	using Super = ui::turbobadger::Window;
	voxel::TreeContext _ctx;

	tb::TBInlineSelect* _trunkHeight;
	tb::TBInlineSelect* _trunkWidth;
	tb::TBInlineSelect* _leavesWidth;
	tb::TBInlineSelect* _leavesHeight;
	tb::TBInlineSelect* _leavesDepth;
public:
	TreeWindow(ui::turbobadger::Window* window, voxel::TreeType type);

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
};

}
