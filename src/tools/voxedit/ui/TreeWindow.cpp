/**
 * @file
 */

#include "TreeWindow.h"

#include "editorscene/ViewportSingleton.h"

namespace voxedit {

TreeWindow::TreeWindow(ui::turbobadger::Window* window, voxel::TreeType type) :
		Super(window) {
	core_assert_always(loadResourceFile("ui/window/voxedit-tree.tb.txt"));

	_trunkHeight     = getWidgetByType<tb::TBInlineSelect>("trunkheight");
	_trunkWidth      = getWidgetByType<tb::TBInlineSelect>("trunkwidth");
	_leavesHeight    = getWidgetByType<tb::TBInlineSelect>("leavesheight");
	_leavesWidth     = getWidgetByType<tb::TBInlineSelect>("leaveswidth");
	_leavesDepth     = getWidgetByType<tb::TBInlineSelect>("leavesdepth");

	if (_trunkHeight == nullptr) {
		Log::error("trunkheight widget not found");
		Close();
		return;
	}
	if (_trunkWidth == nullptr) {
		Log::error("trunkwidth widget not found");
		Close();
		return;
	}
	if (_leavesWidth == nullptr) {
		Log::error("leaveswidth widget not found");
		Close();
		return;
	}
	if (_leavesHeight == nullptr) {
		Log::error("leavesheight widget not found");
		Close();
		return;
	}
	if (_leavesDepth == nullptr) {
		Log::error("leavesdepth widget not found");
		Close();
		return;
	}

	_ctx.type = type;
}

bool TreeWindow::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (ev.target->GetID() == TBIDC("ok")) {
			_ctx.trunkHeight = _trunkHeight->GetValue();
			_ctx.trunkWidth = _trunkWidth->GetValue();
			_ctx.leavesWidth = _leavesWidth->GetValue();
			_ctx.leavesHeight = _leavesHeight->GetValue();
			_ctx.leavesDepth = _leavesDepth->GetValue();
			ViewportSingleton::getInstance().createTree(_ctx);
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
