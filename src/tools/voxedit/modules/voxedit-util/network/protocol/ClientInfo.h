/**
 * @file
 */

#pragma once

#include "core/String.h"
#include <stdint.h>

namespace voxedit {

/**
 * @brief A connected client entry with server-assigned id and display name.
 */
struct ClientInfo {
	uint8_t id;
	core::String name;
};

} // namespace voxedit