/**
 * @file
 */

#pragma once

#include "engine-config.h"

#if USE_GL_RENDERER
#include "gl/GLVersion.h"
#else
#error "No video backend selected"
#endif
