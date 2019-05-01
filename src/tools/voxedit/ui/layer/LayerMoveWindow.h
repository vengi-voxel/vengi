/**
 * @file
 */

#pragma once

#include "AbstractLayerPopupWindow.h"

namespace voxedit {

class LayerMoveWindow : public AbstractLayerPopupWindow {
private:
	using Super = AbstractLayerPopupWindow;
public:
	LayerMoveWindow(tb::TBWidget *target);
	virtual ~LayerMoveWindow() {}

protected:
	void onCreate() override;
};

}
