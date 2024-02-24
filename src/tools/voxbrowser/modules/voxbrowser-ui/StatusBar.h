/**
 * @file
 */

#pragma once

#include "ui/Panel.h"

namespace voxbrowser {

/**
 * @brief Status bar on to the bottom of the main window
 */
class StatusBar : public ui::Panel {
private:
	bool _downloadActive = false;
	float _downloadProgress = 0.0f;
public:
	PANEL_CLASS(StatusBar);
	void update(const char *title, float height, int entries, int allEntries);
	/**
	 * @param value If @c >= 1.0, this indicates that the download is done
	 */
	void downloadProgress(float value);
};

} // namespace voxbrowser
