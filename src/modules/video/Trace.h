/**
 * @file
 */

#pragma once

#include "core/Trace.h"
#include "engine-config.h"
#ifdef TRACY_ENABLE
#ifdef USE_GL_RENDERER
#include "video/gl/flextGL.h"
#include "core/tracy/public/tracy/TracyOpenGL.hpp"
#endif
#ifdef USE_VK_RENDERER
#include "video/vk/VkRenderer.h"
#define VK_NULL_HANDLE nullptr

typedef enum VkTimeDomainEXT {
	VK_TIME_DOMAIN_DEVICE_EXT = 0,
	VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT = 1,
	VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT = 2,
	VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT = 3,
	VK_TIME_DOMAIN_MAX_ENUM_EXT = 0x7FFFFFFF
} VkTimeDomainEXT;

typedef struct VkCalibratedTimestampInfoEXT {
	VkStructureType    sType;
	const void*        pNext;
	VkTimeDomainEXT    timeDomain;
} VkCalibratedTimestampInfoEXT;

typedef VkResult (VKAPI_PTR *PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT)(VkPhysicalDevice physicalDevice, uint32_t* pTimeDomainCount, VkTimeDomainEXT* pTimeDomains);
typedef VkResult (VKAPI_PTR *PFN_vkGetCalibratedTimestampsEXT)(VkDevice device, uint32_t timestampCount, const VkCalibratedTimestampInfoEXT* pTimestampInfos, uint64_t* pTimestamps, uint64_t* pMaxDeviation);

#define VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT (VkStructureType)1000184000

// Stub for vkResetQueryPool to satisfy TracyVulkan.hpp compilation
// This is needed because flextVk doesn't provide it, and TracyVulkan.hpp uses it in a ternary operator.
inline void stub_vkResetQueryPool(VkDevice, VkQueryPool, uint32_t, uint32_t) {}
#define vkResetQueryPool stub_vkResetQueryPool

#include "core/tracy/public/tracy/TracyVulkan.hpp"

#endif
#endif

namespace video {

class VideoTraceScoped {
public:
	VideoTraceScoped(const char* name, const char *msg = nullptr);
	~VideoTraceScoped();
};

extern void traceVideoInit();
extern void traceVideoShutdown();
extern void traceVideoBegin(const char* name);
extern void traceVideoEnd();
extern void traceVideoFrameEnd();

#define video_trace_init() video::traceVideoInit()
#define video_trace_shutdown() video::traceVideoShutdown()
#define video_trace_frame_end() video::traceVideoFrameEnd()
#ifdef TRACY_ENABLE
#define video_trace_begin(name)
#define video_trace_begin_dynamic(name)
#define video_trace_end()
#define video_trace_scoped(name) TracyGpuNamedZone(__tracy_scoped_##name, #name, true)
#else
#define video_trace_begin(name) video::traceVideoBegin(#name)
#define video_trace_begin_dynamic(name) video::traceVideoBegin(#name)
#define video_trace_end() video::traceVideoEnd()
#define video_trace_scoped(name) video::VideoTraceScoped __trace__##name(#name)
#endif

}
