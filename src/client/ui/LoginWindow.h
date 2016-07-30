/**
 * @file
 */

#pragma once

#include "ui/Window.h"
#include "core/Common.h"
#include "../Client.h"
#include "config.h"
#include "SignupWindow.h"

namespace frontend {

// TODO: introduce a login facade here that performs the auto login (if user wishes so), or show this popup. Or forward to a
// signup window
class LoginWindow: public ui::Window {
private:
	Client* _client;

	void doLogin() {
		const std::string& email = getStr("email");
		const std::string& password = getStr("password");

		core::Var::get(cfg::ClientEmail)->setVal(email);
		core::Var::get(cfg::ClientPassword)->setVal(password);

		const core::VarPtr& port = core::Var::get(cfg::ClientPort, SERVER_PORT);
		const core::VarPtr& host = core::Var::get(cfg::ClientHost, SERVER_HOST);
		Log::info("Trying to connect to server %s:%i", host->strVal().c_str(), port->intVal());
		if (!_client->connect(port->intVal(), host->strVal())) {
			Log::info("Failed to connect to server %s:%i", host->strVal().c_str(), port->intVal());
			popup(_("error"), _("failed_to_connect"));
		} else {
			Close();
		}
	}

public:
	LoginWindow(Client* client) :
			ui::Window(client), _client(client) {
		core_assert_always(loadResourceFile("ui/window/login.tb.txt"));
		SetSettings(tb::WINDOW_SETTINGS_TITLEBAR);

		setText("email", core::Var::get(cfg::ClientEmail)->strVal());
		setText("password", core::Var::get(cfg::ClientPassword)->strVal());
		toggleViaVar("autologin", core::Var::get(cfg::ClientAutoLogin));
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
				_app->requestQuit();
				return true;
			} else if (ev.target->GetID() == TBIDC("signup")) {
				new SignupWindow(_client);
				return true;
			} else if (ev.target->GetID() == TBIDC("lostpassword")) {
				popup(_("error"), "not implemented");
				return true;
			} else if (ev.target->GetID() == TBIDC("autologin")) {
				const bool s = isToggled("autologin");
				core::Var::get(cfg::ClientAutoLogin)->setVal(s);
				return true;
			}
		}
		return ui::Window::OnEvent(ev);
	}
};

}
