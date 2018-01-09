/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"
#include "core/Common.h"
#include "util/EMailValidator.h"
#include "../Client.h"

namespace frontend {

class SignupWindow: public ui::turbobadger::Window {
private:
	Client* _client;
public:
	SignupWindow(Client* client) :
			ui::turbobadger::Window(client), _client(client) {
		core_assert_always(loadResourceFile("ui/window/client-signup.tb.txt"));
		SetSettings(tb::WINDOW_SETTINGS_TITLEBAR);
	}

	bool OnEvent(const tb::TBWidgetEvent &ev) override {
		if (ev.type != tb::EVENT_TYPE_CLICK) {
			return ui::turbobadger::Window::OnEvent(ev);
		}
		if (ev.target->GetID() == TBIDC("signup")) {
			const std::string& email = getStr("email");
			const std::string& password = getStr("password");
			const std::string& passwordVerify = getStr("password_verify");
			if (password != passwordVerify) {
				popup(tr("error"), tr("passwordsdonotmatch"));
				return true;
			}
			if (!util::isValidEmail(email)) {
				popup(tr("error"), tr("emailinvalid"));
				return true;
			}
			_client->signup(email, password);
			return true;
		} else if (ev.target->GetID() == TBIDC("cancel")) {
			Close();
			return true;
		}
		return ui::turbobadger::Window::OnEvent(ev);
	}
};

}
