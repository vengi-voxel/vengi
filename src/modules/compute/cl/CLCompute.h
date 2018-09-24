/**
 * @file
 */

#pragma once

#include "CL.h"
#include "CLSymbol.h"
#include "CLMapping.h"
#include "compute/Compute.h"
#include "core/Assert.h"
#include <vector>

namespace compute {

namespace _priv {
struct Context {
	cl_uint platformIdCount = 0;
	std::vector<cl_platform_id> platformIds;
	cl_uint deviceIdCount = 0;
	std::vector<cl_device_id> deviceIds;
	cl_context context = nullptr;
	cl_command_queue commandQueue = nullptr;
	cl_device_id deviceId = nullptr;
	cl_uint alignment = 4096;
	cl_bool imageSupport = CL_FALSE;
	size_t image2DSize[2] = {};
	size_t image3DSize[3] = {};
	std::vector<cl_context_properties> externalProperties;
};

extern Context _ctx;
extern cl_mem_flags convertFlags(BufferFlag flags);
extern bool checkError(cl_int error, bool triggerAssert = true);

}

}
