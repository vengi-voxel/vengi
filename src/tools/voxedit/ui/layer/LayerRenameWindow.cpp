/**
 * @file
 */

#include "LayerRenameWindow.h"

namespace voxedit {

LayerRenameWindow::LayerRenameWindow(tb::TBWidget *target) :
		Super(target, TBIDC("layer_rename_window"), "ui/window/voxedit-layer-rename.tb.txt") {
}

void LayerRenameWindow::onCreate() {
	setText(tr("Rename"));
	addButton("ok", true);
	addButton("cancel", false);
}

}
