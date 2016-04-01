#pragma once

#include "ui/Window.h"
#include "core/Common.h"
#include "../Client.h"

namespace frontend {

class LoginWindow: public ui::Window {
private:
	Client* _client;
public:
	LoginWindow(Client* client) :
			ui::Window(client), _client(client) {
		core_assert(loadResourceFile("ui/window/login.tb.txt"));
		SetSettings(tb::WINDOW_SETTINGS_TITLEBAR);

		const std::string& email = core::Var::get(cfg::ClientEmail)->strVal();
		const std::string& password = core::Var::get(cfg::ClientPassword)->strVal();
		if (!email.empty())
			GetWidgetByID(tb::TBID("email"))->SetText(email.c_str());
		if (!password.empty())
			GetWidgetByID(tb::TBID("password"))->SetText(password.c_str());
	}

	bool OnEvent(const tb::TBWidgetEvent &ev) override {
		if ((ev.type == tb::EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("login")) || ev.special_key == tb::TB_KEY_ENTER) {
			TBWindow *window = ev.target->GetParentWindow();
			TBWidget *email = window->GetWidgetByID(tb::TBID("email"));
			TBWidget *password = window->GetWidgetByID(tb::TBID("password"));

			core::Var::get(cfg::ClientEmail)->setVal(email->GetText().CStr());
			core::Var::get(cfg::ClientPassword)->setVal(password->GetText().CStr());

			const core::VarPtr& port = core::Var::get(cfg::ClientPort, "11337");
			const core::VarPtr& host = core::Var::get(cfg::ClientHost, "127.0.0.1");
			Log::info("Trying to connect to server %s:%i", host->strVal().c_str(), port->intVal());
			if (!_client->connect(port->intVal(), host->strVal())) {
				Log::info("Failed to connect to server %s:%i", host->strVal().c_str(), port->intVal());
				tb::TBStr text;
				text.SetFormatted("Failed to connect");
				tb::TBMessageWindow *win = new tb::TBMessageWindow(this, TBIDC(""));
				win->Show("Failed to connect", text);
			} else {
				Close();
			}
			return true;
		}
		return ui::Window::OnEvent(ev);
	}
};

}
