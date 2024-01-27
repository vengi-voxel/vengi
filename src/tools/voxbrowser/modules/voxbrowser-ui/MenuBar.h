/**
 * @file
 */

#pragma once

#include "ui/IMGUIApp.h"

namespace voxbrowser {

class MenuBar {
private:
	void metricOption();

public:
	bool update(ui::IMGUIApp *app);
};

} // namespace voxbrowser
