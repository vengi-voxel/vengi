/**
 * @file
 */

#pragma once

#include "AbstractLayerPopupWindow.h"

namespace voxedit {

class LayerRenameWindow : public AbstractLayerPopupWindow {
private:
	using Super = AbstractLayerPopupWindow;
public:
	LayerRenameWindow(tb::TBWidget *target);
	virtual ~LayerRenameWindow() {}

protected:
	void onCreate() override;
};

}
