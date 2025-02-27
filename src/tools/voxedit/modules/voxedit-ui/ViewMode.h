/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace voxedit {

enum class ViewMode : uint8_t { Default, Simple, All, CommandAndConquer, Max };

const char *getViewModeString(ViewMode viewMode);

} // namespace voxedit
