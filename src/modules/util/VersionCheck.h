/**
 * @file
 */

#pragma once

#include "core/String.h"

namespace util {

bool isNewVersionAvailable();
bool isNewerVersion(const core::String &versionLatest, const core::String &vengiVersion);

}
