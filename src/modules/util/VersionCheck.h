/**
 * @file
 */

#pragma once

#include "core/String.h"

namespace util {

core::String releaseUrl();
bool isNewVersionAvailable(int timeout = 1);
bool isNewerVersion(const core::String &versionLatest, const core::String &vengiVersion);

}
