/**
 * @file
 */

#pragma once

#include "ui/Window.h"
#include "core/Common.h"
#include "../Client.h"

namespace frontend {

class LostPasswordWindow: public ui::Window {
private:
	Client* _client;

public:
	LostPasswordWindow(Client* client) :
			ui::Window(client), _client(client) {
		core_assert_always(loadResourceFile("ui/window/client-lostpassword.tb.txt"));
		SetSettings(tb::WINDOW_SETTINGS_TITLEBAR);

		setText("email", core::Var::getSafe(cfg::ClientEmail)->strVal());
	}

	bool OnEvent(const tb::TBWidgetEvent &ev) override {
		const std::string& email = getStr("email");
		if (ev.special_key == tb::TB_KEY_ENTER) {
			_client->lostPassword(email);
			Close();
			return true;
		}
		if (ev.type == tb::EVENT_TYPE_CLICK) {
			if (ev.target->GetID() == TBIDC("lostpassword")) {
				_client->lostPassword(email);
				Close();
				return true;
			} else if (ev.target->GetID() == TBIDC("cancel")) {
				Close();
				return true;
			}
		}
		return ui::Window::OnEvent(ev);
	}
};

}
