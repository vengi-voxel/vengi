/**
 * @file
 */

#pragma once

#include "core/String.h"

namespace util {

core::String releaseUrl();
/**
 * @brief Parse a GitHub releases/latest JSON body and compare against PROJECT_VERSION.
 */
bool isNewVersionAvailable(const core::String &responseBody);
bool isNewerVersion(const core::String &versionLatest, const core::String &vengiVersion);

}
