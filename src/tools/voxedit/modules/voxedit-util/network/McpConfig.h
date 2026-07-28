/**
 * @file
 */

#pragma once

#include "core/String.h"

namespace voxedit {

/**
 * @brief Print mcp.json for the current network settings to stdout via Log::printf.
 */
void printMcpConfig(int port, const core::String &iface);

} // namespace voxedit
