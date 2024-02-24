/**
 * @file
 */

#pragma once

#include "ui/Panel.h"

namespace voxbrowser {

class MenuBar : public ui::Panel {
public:
	PANEL_CLASS(MenuBar);
	bool update();

	bool _popupAbout = false;
};

} // namespace voxbrowser
