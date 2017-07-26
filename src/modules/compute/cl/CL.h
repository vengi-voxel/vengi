/**
 * @file
 */
#pragma once

#include <SDL_platform.h>
#ifdef __APPLE__
#include "OpenCL/opencl.h"
#else
#include "CL/cl.h"
#endif
