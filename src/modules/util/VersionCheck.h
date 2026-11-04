/**
 * @file
 */

#pragma once

#include "core/Function.h"
#include "core/String.h"

namespace http {
class Client;
}

namespace util {

core::String releaseUrl();
bool isNewVersionAvailable(const core::String &response);
void checkLatestVersion(http::Client &client, core::Function<void(bool)> callback, int timeout = 1);
bool isNewerVersion(const core::String &versionLatest, const core::String &vengiVersion);

}
