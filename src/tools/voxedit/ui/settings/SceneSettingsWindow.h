/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"

namespace voxedit {

class SceneSettings {
public:
	glm::vec3 diffuseColor;
	glm::vec3 ambientColor;
	glm::vec3 sunPosition;
	glm::vec3 sunDirection;

	std::array<std::string, 4> backgrounds;

	bool diffuseDirty = false;
	bool ambientDirty = false;
	bool sunPositionDirty = false;
	bool sunDirectionDirty = false;
	bool backgroundsDirty = false;
};

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

	tb::TBWidget *getEventDestination() override {
		return _target.get();
	}

	bool show();

	bool onEvent(const tb::TBWidgetEvent &ev) override;
	void onDie() override;
};

}
