/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace voxedit {

#ifdef WIN32
using SocketId = uintptr_t;
#else
using SocketId = int;
#endif
static constexpr SocketId InvalidSocketId = (SocketId)-1;


} // namespace voxedit
