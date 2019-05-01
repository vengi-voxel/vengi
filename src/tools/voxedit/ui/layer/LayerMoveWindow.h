/**
 * @file
 */

#pragma once

#include "AbstractLayerPopupWindow.h"

namespace voxedit {

struct LayerMoveSettings {
	glm::ivec3 move;
};

class LayerMoveWindow : public AbstractLayerPopupWindow {
private:
	using Super = AbstractLayerPopupWindow;
	LayerMoveSettings& _settings;
public:
	LayerMoveWindow(tb::TBWidget *target, LayerMoveSettings& settings) :
		Super(target, TBIDC("layer_move_window"), "ui/window/voxedit-layer-move.tb.txt"), _settings(settings) {
		setText(tr("Move"));
	}
	virtual ~LayerMoveWindow() {}

	bool onEvent(const tb::TBWidgetEvent &ev) override {
		if (ev.type == tb::EVENT_TYPE_CHANGED) {
			if (ev.target->getID() == TBIDC("move.x")) {
				_settings.move.x = atoi(ev.target->getText().c_str());
				return true;
			} else if (ev.target->getID() == TBIDC("move.y")) {
				_settings.move.y = atoi(ev.target->getText().c_str());
				return true;
			} else if (ev.target->getID() == TBIDC("move.z")) {
				_settings.move.z = atoi(ev.target->getText().c_str());
				return true;
			}
		}
		return Super::onEvent(ev);
	}
};

}
