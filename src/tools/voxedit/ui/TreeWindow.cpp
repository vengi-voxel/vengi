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
		close();
		return;
	}
	if (_trunkWidth == nullptr) {
		Log::error("trunkwidth widget not found");
		close();
		return;
	}
	if (_leavesWidth == nullptr) {
		Log::error("leaveswidth widget not found");
		close();
		return;
	}
	if (_leavesHeight == nullptr) {
		Log::error("leavesheight widget not found");
		close();
		return;
	}
	if (_leavesDepth == nullptr) {
		Log::error("leavesdepth widget not found");
		close();
		return;
	}

	_ctx.type = type;
}

bool TreeWindow::onEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (ev.target->getID() == TBIDC("ok")) {
			_ctx.trunkHeight = _trunkHeight->getValue();
			_ctx.trunkWidth = _trunkWidth->getValue();
			_ctx.leavesWidth = _leavesWidth->getValue();
			_ctx.leavesHeight = _leavesHeight->getValue();
			_ctx.leavesDepth = _leavesDepth->getValue();
			ViewportSingleton::getInstance().createTree(_ctx);
			close();
			return true;
		} else if (ev.target->getID() == TBIDC("cancel")) {
			close();
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_KEY_DOWN) {
		if (ev.special_key == tb::TB_KEY_ESC) {
			close();
			return true;
		}
	}
	return Super::onEvent(ev);
}

}
