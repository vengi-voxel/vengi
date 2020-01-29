/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"
#include "core/Common.h"
#include "../Client.h"
#include "engine-config.h"
#include "SignupWindow.h"
#include "LostPasswordWindow.h"

namespace frontend {

// TODO: introduce a login facade here that performs the auto login (if user wishes so), or show this popup. Or forward to a
// signup window
class LoginWindow: public ui::turbobadger::Window {
private:
	using Super = ui::turbobadger::Window;
	Client* _client;

	void doLogin() {
		const core::String& email = getStr("email");
		const core::String& password = getStr("password");

		core::Var::get(cfg::ClientEmail, email.c_str())->setVal(email);
		core::Var::get(cfg::ClientPassword, password.c_str())->setVal(password);

		const core::VarPtr& port = core::Var::getSafe(cfg::ClientPort);
		const core::VarPtr& host = core::Var::getSafe(cfg::ClientHost);
		if (!_client->connect(port->intVal(), host->strVal())) {
			Log::info("Failed to connect to server %s:%i", host->strVal().c_str(), port->intVal());
			popup(tr("error"), tr("failed_to_connect"));
		} else {
			close();
		}
	}

public:
	LoginWindow(Client* client) :
			Super(client), _client(client) {
		core_assert_always(loadResourceFile("ui/window/client-login.tb.txt"));
		setSettings(tb::WINDOW_SETTINGS_TITLEBAR);

		setStr("email", core::Var::str(cfg::ClientEmail));
		setStr("password", core::Var::str(cfg::ClientPassword));
		toggle("autologin", core::Var::boolean(cfg::ClientAutoLogin));
	}

	bool onEvent(const tb::TBWidgetEvent &ev) override {
		if (ev.special_key == tb::TB_KEY_ENTER) {
			// switch focus to finish any cvar editing step
			if (TBWidget* login = getWidgetByID("login")) {
				login->setFocus(tb::WIDGET_FOCUS_REASON_UNKNOWN);
			}
			doLogin();
			return true;
		}
		if (ev.type == tb::EVENT_TYPE_CLICK) {
			if (ev.target->getID() == TBIDC("login")) {
				doLogin();
				return true;
			} else if (ev.target->getID() == TBIDC("cancel")) {
				requestQuit();
				return true;
			} else if (ev.target->getID() == TBIDC("signup")) {
				new SignupWindow(_client);
				return true;
			} else if (ev.target->getID() == TBIDC("lostpassword")) {
				new LostPasswordWindow(_client);
				return true;
			} else if (ev.target->getID() == TBIDC("autologin")) {
				const bool s = isToggled("autologin");
				core::Var::getSafe(cfg::ClientAutoLogin)->setVal(s);
				return true;
			}
		}
		return Super::onEvent(ev);
	}
};

}
