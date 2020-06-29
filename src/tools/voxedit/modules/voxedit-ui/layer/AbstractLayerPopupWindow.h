/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"

namespace voxedit {

class AbstractLayerPopupWindow : public tb::TBWindow, private tb::TBWidgetListener {
private:
	using Super = tb::TBWindow;
	tb::TBWidgetSafePointer _dimmer;
	tb::TBWidgetSafePointer _target;
	const char *_file;

protected:
	virtual void onShow() {}
	void addButton(const tb::TBID &id, bool focused);
	void onWidgetDelete(tb::TBWidget *widget) override;
	bool onWidgetDying(tb::TBWidget *widget) override;

public:
	AbstractLayerPopupWindow(tb::TBWidget *target, const tb::TBID &id, const char *file);
	virtual ~AbstractLayerPopupWindow();

	tb::TBWidget *getEventDestination() const override {
		return _target.get();
	}

	bool show();

	bool onEvent(const tb::TBWidgetEvent &ev) override;
	void onDie() override;
};

}
