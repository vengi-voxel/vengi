/**
 * @file
 */

#pragma once

#include "AbstractLayerPopupWindow.h"

namespace voxedit {

struct LayerRenameSettings {
	std::string name;
};

class LayerRenameWindow : public AbstractLayerPopupWindow {
private:
	using Super = AbstractLayerPopupWindow;
	LayerRenameSettings& _settings;
protected:
	void onShow() override {
		if (tb::TBEditField* f = getWidgetByIDAndType<tb::TBEditField>("name")) {
			f->setText(_settings.name.c_str());
		}
	}
public:
	LayerRenameWindow(tb::TBWidget *target, LayerRenameSettings& settings) :
		Super(target, TBIDC("layer_rename_window"), "ui/window/voxedit-layer-rename.tb.txt"), _settings(settings) {
		setText(tr("Rename"));
	}
	virtual ~LayerRenameWindow() {}

	bool onEvent(const tb::TBWidgetEvent &ev) override {
		if (ev.type == tb::EVENT_TYPE_CHANGED && ev.target->getID() == TBIDC("name")) {
			_settings.name = ev.target->getText();
			return true;
		}
		return Super::onEvent(ev);
	}
};

}
