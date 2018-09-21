/**
 * @file
 */
#pragma once

#include <SDL_platform.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CL_TARGET_OPENCL_VERSION 120
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#ifdef __APPLE__
#include "OpenCL/opencl.h"
#else
#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <CL/cl_gl.h>
#endif

#ifdef __cplusplus
}
#endif
