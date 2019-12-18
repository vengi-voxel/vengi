/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"
#include "core/Common.h"
#include "../Client.h"

namespace frontend {

class HudWindow: public ui::turbobadger::Window {
private:
	using Super = ui::turbobadger::Window;
	Client* _client;
public:
	HudWindow(Client* client, const glm::ivec2& dimension) :
			Super(client), _client(client) {
		core_assert_always(loadResourceFile("ui/window/client-hud.tb.txt"));
		setSettings(tb::WINDOW_SETTINGS_NONE);
		setSize(dimension.x, 20);
	}

	bool onEvent(const tb::TBWidgetEvent &ev) override {
#if 0
		if (ev.type == tb::EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("disconnect")) {
			_client->disconnect();
			return true;
		}
#endif
		return Super::onEvent(ev);
	}
};

}
