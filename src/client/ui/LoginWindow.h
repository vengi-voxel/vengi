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
class LoginWindow: public ui::Window {
private:
	Client* _client;

	void doLogin() {
		const std::string& email = getStr("email");
		const std::string& password = getStr("password");

		core::Var::get(cfg::ClientEmail, email.c_str())->setVal(email);
		core::Var::get(cfg::ClientPassword, password.c_str())->setVal(password);

		const core::VarPtr& port = core::Var::getSafe(cfg::ClientPort);
		const core::VarPtr& host = core::Var::getSafe(cfg::ClientHost);
		Log::info("Trying to connect to server %s:%i", host->strVal().c_str(), port->intVal());
		if (!_client->connect(port->intVal(), host->strVal())) {
			Log::info("Failed to connect to server %s:%i", host->strVal().c_str(), port->intVal());
			popup(tr("error"), tr("failed_to_connect"));
		} else {
			Close();
		}
	}

public:
	LoginWindow(Client* client) :
			ui::Window(client), _client(client) {
		core_assert_always(loadResourceFile("ui/window/client-login.tb.txt"));
		SetSettings(tb::WINDOW_SETTINGS_TITLEBAR);

		setText("email", core::Var::str(cfg::ClientEmail));
		setText("password", core::Var::str(cfg::ClientPassword));
		toggle("autologin", core::Var::boolean(cfg::ClientAutoLogin));
	}

	bool OnEvent(const tb::TBWidgetEvent &ev) override {
		if (ev.special_key == tb::TB_KEY_ENTER) {
			doLogin();
			return true;
		}
		if (ev.type == tb::EVENT_TYPE_CLICK) {
			if (ev.target->GetID() == TBIDC("login")) {
				doLogin();
				return true;
			} else if (ev.target->GetID() == TBIDC("cancel")) {
				requestQuit();
				return true;
			} else if (ev.target->GetID() == TBIDC("signup")) {
				new SignupWindow(_client);
				return true;
			} else if (ev.target->GetID() == TBIDC("lostpassword")) {
				new LostPasswordWindow(_client);
				return true;
			} else if (ev.target->GetID() == TBIDC("autologin")) {
				const bool s = isToggled("autologin");
				core::Var::getSafe(cfg::ClientAutoLogin)->setVal(s);
				return true;
			}
		}
		return ui::Window::OnEvent(ev);
	}
};

}
