/**
 * @file
 */

#include "LayerMoveWindow.h"

namespace voxedit {

LayerMoveWindow::LayerMoveWindow(tb::TBWidget *target) :
		Super(target, TBIDC("layer_move_window"), "ui/window/voxedit-layer-move.tb.txt") {
}

void LayerMoveWindow::onCreate() {
	setText(tr("Move"));
	addButton("ok", true);
	addButton("cancel", false);
}

}
