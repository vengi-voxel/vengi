#pragma once

#include "TurboBadger.h"
#include <SDL.h>

namespace ui {

class Widget : public tb::TBWidget {
protected:
	inline bool isRelativeMouseMode() const {
		return SDL_GetRelativeMouseMode() == SDL_TRUE ? true : false;
	}
public:
	Widget() :
			tb::TBWidget() {
	}
};

}
