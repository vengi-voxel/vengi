/**
 * @file
 */

#include "LayerWindow.h"
#include "core/Assert.h"
#include "app/App.h"
#include "core/StringUtil.h"
#include "io/Filesystem.h"
#include "tb_message_window.h"
#include "tb_editfield.h"
#include "tb_language.h"
#include "tb_widgets_reader.h"

namespace voxedit {

LayerWindow::LayerWindow(tb::TBWidget *target, const tb::TBID &id, LayerSettings& layerSettings, LayerWindowSettings* settings) :
		Super(target, id, "ui/window/voxedit-layer.tb.txt"), _layerSettings(layerSettings) {
	if (settings != nullptr) {
		_layerWindowSettings = *settings;
	}
}

void LayerWindow::onShow() {
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

	if (_layerWindowSettings.type == LayerWindowType::NewScene) {
		setText(tr("New Scene"));
	} else if (_layerWindowSettings.type == LayerWindowType::Create) {
		setText(tr("New Layer"));
	} else if (_layerWindowSettings.type == LayerWindowType::Edit) {
		setText(tr("Edit Layer"));
	}
}

void LayerWindow::checkSize() {
	if (tb::TBWidget* f = getWidgetByID("note")) {
		if (_layerSettings.size.x <= voxedit::_priv::MaxVolumeSize && _layerSettings.size.y <= voxedit::_priv::MaxVolumeSize && _layerSettings.size.z <= voxedit::_priv::MaxVolumeSize) {
			f->setText("");
			return;
		}
		static_assert(_priv::MaxVolumeSize == 256, "Expected the max volume size to be 256");
		f->setText(tr("Volume size can't get saved to e.g. vox file format. Max value is 256 for the size."));
	}
}

bool LayerWindow::onEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CHANGED) {
		if (ev.target->getID() == TBIDC("pos.x")) {
			_layerSettings.position.x = core::string::toInt(ev.target->getText().c_str());
			return true;
		} else if (ev.target->getID() == TBIDC("pos.y")) {
			_layerSettings.position.y = core::string::toInt(ev.target->getText().c_str());
			return true;
		} else if (ev.target->getID() == TBIDC("pos.z")) {
			_layerSettings.position.z = core::string::toInt(ev.target->getText().c_str());
			return true;
		} else if (ev.target->getID() == TBIDC("size.x")) {
			_layerSettings.size.x = core::string::toInt(ev.target->getText().c_str());
			checkSize();
			return true;
		} else if (ev.target->getID() == TBIDC("size.y")) {
			_layerSettings.size.y = core::string::toInt(ev.target->getText().c_str());
			checkSize();
			return true;
		} else if (ev.target->getID() == TBIDC("size.z")) {
			_layerSettings.size.z = core::string::toInt(ev.target->getText().c_str());
			checkSize();
			return true;
		} else if (ev.target->getID() == TBIDC("name")) {
			_layerSettings.name = ev.target->getText().c_str();
			return true;
		}
	}
	return Super::onEvent(ev);
}

}
