/**
 * @file
 */

#include "SceneSettingsWindow.h"
#include "core/Assert.h"
#include "core/App.h"
#include "io/Filesystem.h"
#include "tb_message_window.h"
#include "tb_editfield.h"
#include "tb_language.h"
#include "tb_widgets_reader.h"

namespace voxedit {

SceneSettingsWindow::SceneSettingsWindow(tb::TBWidget *target, SceneSettings* settings) :
		_target(target), _settings(settings) {
	tb::TBWidgetListener::addGlobalListener(this);
	setID(TBIDC("scene_settings"));
}

SceneSettingsWindow::~SceneSettingsWindow() {
	tb::TBWidgetListener::removeGlobalListener(this);
	if (tb::TBWidget *dimmer = _dimmer.get()) {
		dimmer->removeFromParent();
		delete dimmer;
	}
}

bool SceneSettingsWindow::show() {
	tb::TBWidget *target = _target.get();
	if (target == nullptr) {
		return false;
	}

	tb::TBWidget *root = target->getParentRoot();

	const std::string& source = core::App::getInstance()->filesystem()->load("ui/window/voxedit-scene-settings.tb.txt");
	if (!tb::g_widgets_reader->loadData(getContentRoot(), source.c_str())) {
		return false;
	}

	onShow();

	addButton("ok", true);
	addButton("cancel", false);

	resizeToFitContent();

	if (tb::TBDimmer *dimmer = new tb::TBDimmer()) {
		root->addChild(dimmer);
		_dimmer.set(dimmer);
	}

	tb::TBRect rect = getRect();
	tb::TBRect bounds(0, 0, root->getRect().w, root->getRect().h);
	setRect(rect.centerIn(bounds).moveIn(bounds).clip(bounds));
	root->addChild(this);
	return true;
}

void SceneSettingsWindow::addButton(const tb::TBID &id, bool focused) {
	tb::TBLayout *layout = getWidgetByIDAndType<tb::TBLayout>(TBIDC("buttons"));
	if (layout == nullptr) {
		return;
	}
	if (tb::TBButton *btn = new tb::TBButton()) {
		btn->setID(id);
		btn->setText(tb::g_tb_lng->getString(btn->getID()));
		layout->addChild(btn);
		if (focused) {
			btn->setFocus(tb::WIDGET_FOCUS_REASON_UNKNOWN);
		}
	}
}

bool SceneSettingsWindow::onEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK && ev.target->isOfType<tb::TBButton>()) {
		tb::TBWidgetSafePointer this_widget(this);

		// Invoke the click on the target
		tb::TBWidgetEvent targetEvent(tb::EVENT_TYPE_CLICK);
		targetEvent.ref_id = ev.target->getID();
		invokeEvent(targetEvent);

		// If target got deleted, close
		if (this_widget.get() != nullptr) {
			close();
		}
		return true;
	}
	if (ev.type == tb::EVENT_TYPE_KEY_DOWN && ev.special_key == tb::TB_KEY_ESC) {
		tb::TBWidgetEvent clickEvent(tb::EVENT_TYPE_CLICK);
		m_close_button.invokeEvent(clickEvent);
		return true;
	}
	return Super::onEvent(ev);
}

void SceneSettingsWindow::onDie() {
	if (tb::TBWidget *dimmer = _dimmer.get()) {
		dimmer->die();
	}
}

void SceneSettingsWindow::onWidgetDelete(tb::TBWidget *widget) {
	// If the target widget is deleted, close!
	if (_target.get() == nullptr) {
		close();
	}
}

bool SceneSettingsWindow::onWidgetDying(tb::TBWidget *widget) {
	// If the target widget or an ancestor of it is dying, close!
	if (widget == _target.get() || widget->isAncestorOf(_target.get())) {
		close();
	}
	return false;
}

}
