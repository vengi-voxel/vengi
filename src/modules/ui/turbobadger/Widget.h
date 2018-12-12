#pragma once

#include "TurboBadger.h"
#include <SDL.h>

namespace ui {
namespace turbobadger {

class Widget : public tb::TBWidget {
protected:
	inline bool isRelativeMouseMode() const {
		return SDL_GetRelativeMouseMode() == SDL_TRUE ? true : false;
	}

	inline bool isMiddleMouseButtonPressed() const {
		return SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_MIDDLE);
	}

	inline bool isRightMouseButtonPressed() const {
		return SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_RIGHT);
	}
public:
	Widget() :
			tb::TBWidget() {
	}

	~Widget() override {}
};

}
}
