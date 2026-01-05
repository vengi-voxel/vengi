/**
 * @file
 */

#pragma once

#include "GLVersion.h"
#include "video/RendererState.h"

namespace video {

namespace _priv {

struct GLState : public RendererState {
	GLVersion glVersion{0, 0};
};

} // namespace _priv

} // namespace video
