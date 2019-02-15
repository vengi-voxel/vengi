/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"
#include "core/Common.h"

namespace frontend {

class AuthFailedWindow: public ui::turbobadger::Window {
private:
	using Super = ui::turbobadger::Window;
public:
	AuthFailedWindow(Window* parent) :
		Super(parent) {
		core_assert_always(loadResourceFile("ui/window/client-authfailed.tb.txt"));
	}

	bool onEvent(const tb::TBWidgetEvent &ev) override {
		if (ev.type == tb::EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("ok")) {
			close();
			return true;
		}
		return Super::onEvent(ev);
	}
};

}
