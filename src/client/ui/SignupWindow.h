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
	using Super = ui::turbobadger::Window;
	Client* _client;
public:
	SignupWindow(Client* client) :
			Super(client), _client(client) {
		core_assert_always(loadResourceFile("ui/window/client-signup.tb.txt"));
		setSettings(tb::WINDOW_SETTINGS_TITLEBAR);
	}

	bool onEvent(const tb::TBWidgetEvent &ev) override {
		if (ev.type != tb::EVENT_TYPE_CLICK) {
			return Super::onEvent(ev);
		}
		if (ev.target->getID() == TBIDC("signup")) {
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
		} else if (ev.target->getID() == TBIDC("cancel")) {
			close();
			return true;
		}
		return Super::onEvent(ev);
	}
};

}
