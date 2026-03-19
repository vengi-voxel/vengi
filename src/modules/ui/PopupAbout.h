/**
 * @file
 */

#pragma once

#include "core/Function.h"

#define POPUP_TITLE_ABOUT "###popuptitle"

namespace ui {

void metricOption();
void popupAbout(const core::Function<void()> &customTabs = {}, bool isNewVersionAvailable = false);

} // namespace ui
