/**
 * @file
 */

#pragma once

#include <functional>

#define POPUP_TITLE_ABOUT "About##popuptitle"

namespace ui {

void metricOption();
void popupAbout(const std::function<void()> &customTabs = {}, bool isNewVersionAvailable = false);

} // namespace ui
