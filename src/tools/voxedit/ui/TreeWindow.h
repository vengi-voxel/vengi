/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"
#include "voxel/TreeContext.h"

class EditorScene;

namespace voxedit {

class TreeWindow : public ui::Window {
private:
	using Super = ui::Window;
	EditorScene* _scene;
	voxel::TreeContext _ctx;

	tb::TBInlineSelect* _trunkHeight;
	tb::TBInlineSelect* _trunkWidth;
	tb::TBInlineSelect* _leavesWidth;
	tb::TBInlineSelect* _leavesHeight;
	tb::TBInlineSelect* _leavesDepth;
public:
	TreeWindow(ui::Window* window, EditorScene* scene, voxel::TreeType type);

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
};

}
