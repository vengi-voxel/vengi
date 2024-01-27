/**
 * @file
 */

#pragma once

namespace voxbrowser {

/**
 * @brief Status bar on to the bottom of the main window
 */
class StatusBar {
public:
	void update(const char *title, float height);
};

} // namespace voxbrowser
