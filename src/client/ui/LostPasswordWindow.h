/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"
#include "core/Common.h"
#include "../Client.h"

namespace frontend {

class LostPasswordWindow: public ui::turbobadger::Window {
private:
	using Super = ui::turbobadger::Window;
	Client* _client;

public:
	LostPasswordWindow(Client* client) :
			Super(client), _client(client) {
		core_assert_always(loadResourceFile("ui/window/client-lostpassword.tb.txt"));
		setSettings(tb::WINDOW_SETTINGS_TITLEBAR);

		setStr("email", core::Var::getSafe(cfg::ClientEmail)->strVal());
	}

	bool onEvent(const tb::TBWidgetEvent &ev) override {
		const core::String& email = getStr("email");
		if (ev.special_key == tb::TB_KEY_ENTER) {
			_client->lostPassword(email);
			close();
			return true;
		}
		if (ev.type == tb::EVENT_TYPE_CLICK) {
			if (ev.target->getID() == TBIDC("lostpassword")) {
				_client->lostPassword(email);
				close();
				return true;
			} else if (ev.target->getID() == TBIDC("cancel")) {
				close();
				return true;
			}
		}
		return Super::onEvent(ev);
	}
};

}
