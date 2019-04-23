/**
 * @file
 */

#include "LayerWindow.h"
#include "core/Assert.h"
#include "core/App.h"
#include "io/Filesystem.h"
#include "tb_message_window.h"
#include "tb_editfield.h"
#include "tb_language.h"
#include "tb_widgets_reader.h"

namespace voxedit {

LayerWindow::LayerWindow(TBWidget *target, const tb::TBID &id, LayerSettings& layerSettings) :
		_target(target), _layerSettings(layerSettings) {
	TBWidgetListener::addGlobalListener(this);
	setID(id);
}

LayerWindow::~LayerWindow() {
	TBWidgetListener::removeGlobalListener(this);
	if (TBWidget *dimmer = _dimmer.get()) {
		dimmer->removeFromParent();
		delete dimmer;
	}
}

bool LayerWindow::show(LayerWindowSettings* settings) {
	TBWidget *target = _target.get();
	if (target == nullptr) {
		return false;
	}

	TBWidget *root = target->getParentRoot();

	const std::string& source = core::App::getInstance()->filesystem()->load("ui/window/voxedit-layer.tb.txt");
	if (!tb::g_widgets_reader->loadData(getContentRoot(), source.c_str())) {
		return false;
	}

	if (tb::TBEditField* f = getWidgetByIDAndType<tb::TBEditField>("pos.x")) {
		f->setTextFormatted("%i", _layerSettings.position.x);
	}
	if (tb::TBEditField* f = getWidgetByIDAndType<tb::TBEditField>("pos.y")) {
		f->setTextFormatted("%i", _layerSettings.position.y);
	}
	if (tb::TBEditField* f = getWidgetByIDAndType<tb::TBEditField>("pos.z")) {
		f->setTextFormatted("%i", _layerSettings.position.z);
	}

	if (tb::TBEditField* f = getWidgetByIDAndType<tb::TBEditField>("size.x")) {
		f->setTextFormatted("%i", _layerSettings.size.x);
	}
	if (tb::TBEditField* f = getWidgetByIDAndType<tb::TBEditField>("size.y")) {
		f->setTextFormatted("%i", _layerSettings.size.y);
	}
	if (tb::TBEditField* f = getWidgetByIDAndType<tb::TBEditField>("size.z")) {
		f->setTextFormatted("%i", _layerSettings.size.z);
	}
	if (tb::TBEditField* f = getWidgetByIDAndType<tb::TBEditField>("name")) {
		f->setText(_layerSettings.name.c_str());
	}

	LayerWindowSettings defaultSettings;
	if (settings == nullptr) {
		settings = &defaultSettings;
	}

	if (settings->type == LayerWindowType::NewScene) {
		setText(tr("New Scene"));
	} else if (settings->type == LayerWindowType::Create) {
		setText(tr("New Layer"));
	} else if (settings->type == LayerWindowType::Edit) {
		setText(tr("Edit Layer"));
	}

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

void LayerWindow::addButton(const tb::TBID &id, bool focused) {
	tb::TBLayout *layout = getWidgetByIDAndType<tb::TBLayout>(4);
	if (layout == nullptr) {
		return;
	}
	if (tb::TBButton *btn = new tb::TBButton) {
		btn->setID(id);
		btn->setText(tb::g_tb_lng->getString(btn->getID()));
		layout->addChild(btn);
		if (focused) {
			btn->setFocus(tb::WIDGET_FOCUS_REASON_UNKNOWN);
		}
	}
}

bool LayerWindow::onEvent(const tb::TBWidgetEvent &ev) {
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
	if (ev.type == tb::EVENT_TYPE_CHANGED) {
		if (ev.target->getID() == TBIDC("pos.x")) {
			_layerSettings.position.x = atoi(ev.target->getText().c_str());
			return true;
		} else if (ev.target->getID() == TBIDC("pos.y")) {
			_layerSettings.position.y = atoi(ev.target->getText().c_str());
			return true;
		} else if (ev.target->getID() == TBIDC("pos.z")) {
			_layerSettings.position.z = atoi(ev.target->getText().c_str());
			return true;
		} else if (ev.target->getID() == TBIDC("size.x")) {
			_layerSettings.size.x = atoi(ev.target->getText().c_str());
			return true;
		} else if (ev.target->getID() == TBIDC("size.y")) {
			_layerSettings.size.y = atoi(ev.target->getText().c_str());
			return true;
		} else if (ev.target->getID() == TBIDC("size.z")) {
			_layerSettings.size.z = atoi(ev.target->getText().c_str());
			return true;
		} else if (ev.target->getID() == TBIDC("name")) {
			_layerSettings.name = ev.target->getText().c_str();
			return true;
		}
	}
	if (ev.type == tb::EVENT_TYPE_KEY_DOWN && ev.special_key == tb::TB_KEY_ESC) {
		tb::TBWidgetEvent clickEvent(tb::EVENT_TYPE_CLICK);
		m_close_button.invokeEvent(clickEvent);
		return true;
	}
	return Super::onEvent(ev);
}

void LayerWindow::onDie() {
	if (TBWidget *dimmer = _dimmer.get()) {
		dimmer->die();
	}
}

void LayerWindow::onWidgetDelete(TBWidget *widget) {
	// If the target widget is deleted, close!
	if (_target.get() == nullptr) {
		close();
	}
}

bool LayerWindow::onWidgetDying(TBWidget *widget) {
	// If the target widget or an ancestor of it is dying, close!
	if (widget == _target.get() || widget->isAncestorOf(_target.get())) {
		close();
	}
	return false;
}

}
