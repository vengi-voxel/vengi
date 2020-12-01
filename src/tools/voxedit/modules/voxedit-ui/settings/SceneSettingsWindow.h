/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"
#include "voxedit-util/SceneSettings.h"

namespace voxedit {

class SceneSettingsWindow : public tb::TBWindow, private tb::TBWidgetListener {
private:
	using Super = tb::TBWindow;
	tb::TBWidgetSafePointer _dimmer;
	tb::TBWidgetSafePointer _target;
	SceneSettings* _settings;

protected:
	void addButton(const tb::TBID &id, bool focused);
	void onWidgetDelete(tb::TBWidget *widget) override;
	bool onWidgetDying(tb::TBWidget *widget) override;

public:
	SceneSettingsWindow(tb::TBWidget *target, SceneSettings* settings);
	virtual ~SceneSettingsWindow();

	tb::TBWidget *getEventDestination() const override {
		return _target.get();
	}

	bool show();

	bool onEvent(const tb::TBWidgetEvent &ev) override;
	void onDie() override;
};

}
