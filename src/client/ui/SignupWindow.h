/**
 * @file
 */

#pragma once

#include "ui/Window.h"
#include "core/Common.h"
#include "../Client.h"

namespace frontend {

class SignupWindow: public ui::Window {
private:
	Client* _client;
public:
	SignupWindow(Client* client) :
			ui::Window(client), _client(client) {
		core_assert_always(loadResourceFile("ui/window/signup.tb.txt"));
		SetSettings(tb::WINDOW_SETTINGS_TITLEBAR);
	}

	bool OnEvent(const tb::TBWidgetEvent &ev) override {
		if (ev.type == tb::EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("signup")) {
			const std::string& email = getStr("email");
			const std::string& password = getStr("password");
			const std::string& passwordVerify = getStr("password_verify");
			return true;
		}
		return ui::Window::OnEvent(ev);
	}
};

}
