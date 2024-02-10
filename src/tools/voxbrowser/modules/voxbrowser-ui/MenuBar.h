/**
 * @file
 */

#pragma once

#include "ui/IMGUIApp.h"

namespace voxbrowser {

class MenuBar {
public:
	bool update(ui::IMGUIApp *app);

	bool _popupAbout = false;
};

} // namespace voxbrowser
