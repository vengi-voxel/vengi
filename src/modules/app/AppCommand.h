/**
 * @file
 */

#pragma once

#include "core/TimeProvider.h"

namespace app {
namespace AppCommand {
/**
 * Adds a list of default commands for apps
 */
extern void init(const core::TimeProviderPtr& timeProvider);
}
}
