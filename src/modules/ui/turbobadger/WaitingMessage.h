#pragma once

#include "TurboBadger.h"
#include "core/Color.h"

namespace ui {
namespace turbobadger {

class UIApp;

class WaitingMessage {
private:
	UIApp* _app;
	tb::TBFontFace *_font = nullptr;
	tb::TBColor _color = {255, 255, 255, 255};
	const char* _translatedStr = nullptr;
	uint64_t _connectingStart = 0ul;
	int _dotsIndex = 0;
public:
	WaitingMessage(UIApp* app);
	~WaitingMessage();

	void init(int fontSize = 28);
	void shutdown();

	void setColor(const glm::vec4& color);
	/**
	 * @param[in] textId The language identifier
	 */
	void setTextId(const char *textId);

	void reset();
	void update(uint64_t deltaFrame);
	void render();
};

}
}
