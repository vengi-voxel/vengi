/**
 * @brief
 */

#pragma once

#include <stdint.h>

namespace scenegraph {

using FrameIndex = int32_t;
using KeyFrameIndex = int32_t;
#define InvalidKeyFrame ((scenegraph::KeyFrameIndex)-1)
#define InvalidFrame ((scenegraph::FrameIndex)-1)

} // namespace scenegraph
