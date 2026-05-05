/* WARNING: This file was automatically generated */
/* Do not edit. */

#include "flextVk.h"
#include "core/sdl/SDLSystem.h"

#ifdef __cplusplus
extern "C" {
#endif

PFNVKALLOCATECOMMANDBUFFERS_PROC* flextvkAllocateCommandBuffers = NULL;
PFNVKALLOCATEMEMORY_PROC* flextvkAllocateMemory = NULL;
PFNVKBEGINCOMMANDBUFFER_PROC* flextvkBeginCommandBuffer = NULL;
PFNVKBINDBUFFERMEMORY_PROC* flextvkBindBufferMemory = NULL;
PFNVKBINDIMAGEMEMORY_PROC* flextvkBindImageMemory = NULL;
PFNVKCMDBEGINQUERY_PROC* flextvkCmdBeginQuery = NULL;
PFNVKCMDCOPYBUFFER_PROC* flextvkCmdCopyBuffer = NULL;
PFNVKCMDCOPYBUFFERTOIMAGE_PROC* flextvkCmdCopyBufferToImage = NULL;
PFNVKCMDCOPYIMAGE_PROC* flextvkCmdCopyImage = NULL;
PFNVKCMDCOPYIMAGETOBUFFER_PROC* flextvkCmdCopyImageToBuffer = NULL;
PFNVKCMDCOPYQUERYPOOLRESULTS_PROC* flextvkCmdCopyQueryPoolResults = NULL;
PFNVKCMDENDQUERY_PROC* flextvkCmdEndQuery = NULL;
PFNVKCMDEXECUTECOMMANDS_PROC* flextvkCmdExecuteCommands = NULL;
PFNVKCMDFILLBUFFER_PROC* flextvkCmdFillBuffer = NULL;
PFNVKCMDPIPELINEBARRIER_PROC* flextvkCmdPipelineBarrier = NULL;
PFNVKCMDRESETQUERYPOOL_PROC* flextvkCmdResetQueryPool = NULL;
PFNVKCMDUPDATEBUFFER_PROC* flextvkCmdUpdateBuffer = NULL;
PFNVKCMDWRITETIMESTAMP_PROC* flextvkCmdWriteTimestamp = NULL;
PFNVKCREATEBUFFER_PROC* flextvkCreateBuffer = NULL;
PFNVKCREATECOMMANDPOOL_PROC* flextvkCreateCommandPool = NULL;
PFNVKCREATEDEVICE_PROC* flextvkCreateDevice = NULL;
PFNVKCREATEFENCE_PROC* flextvkCreateFence = NULL;
PFNVKCREATEIMAGE_PROC* flextvkCreateImage = NULL;
PFNVKCREATEIMAGEVIEW_PROC* flextvkCreateImageView = NULL;
PFNVKCREATEINSTANCE_PROC* flextvkCreateInstance = NULL;
PFNVKCREATEQUERYPOOL_PROC* flextvkCreateQueryPool = NULL;
PFNVKCREATESEMAPHORE_PROC* flextvkCreateSemaphore = NULL;
PFNVKDESTROYBUFFER_PROC* flextvkDestroyBuffer = NULL;
PFNVKDESTROYCOMMANDPOOL_PROC* flextvkDestroyCommandPool = NULL;
PFNVKDESTROYDEVICE_PROC* flextvkDestroyDevice = NULL;
PFNVKDESTROYFENCE_PROC* flextvkDestroyFence = NULL;
PFNVKDESTROYIMAGE_PROC* flextvkDestroyImage = NULL;
PFNVKDESTROYIMAGEVIEW_PROC* flextvkDestroyImageView = NULL;
PFNVKDESTROYINSTANCE_PROC* flextvkDestroyInstance = NULL;
PFNVKDESTROYQUERYPOOL_PROC* flextvkDestroyQueryPool = NULL;
PFNVKDESTROYSEMAPHORE_PROC* flextvkDestroySemaphore = NULL;
PFNVKDEVICEWAITIDLE_PROC* flextvkDeviceWaitIdle = NULL;
PFNVKENDCOMMANDBUFFER_PROC* flextvkEndCommandBuffer = NULL;
PFNVKENUMERATEDEVICEEXTENSIONPROPERTIES_PROC* flextvkEnumerateDeviceExtensionProperties = NULL;
PFNVKENUMERATEDEVICELAYERPROPERTIES_PROC* flextvkEnumerateDeviceLayerProperties = NULL;
PFNVKENUMERATEINSTANCEEXTENSIONPROPERTIES_PROC* flextvkEnumerateInstanceExtensionProperties = NULL;
PFNVKENUMERATEINSTANCELAYERPROPERTIES_PROC* flextvkEnumerateInstanceLayerProperties = NULL;
PFNVKENUMERATEPHYSICALDEVICES_PROC* flextvkEnumeratePhysicalDevices = NULL;
PFNVKFLUSHMAPPEDMEMORYRANGES_PROC* flextvkFlushMappedMemoryRanges = NULL;
PFNVKFREECOMMANDBUFFERS_PROC* flextvkFreeCommandBuffers = NULL;
PFNVKFREEMEMORY_PROC* flextvkFreeMemory = NULL;
PFNVKGETBUFFERMEMORYREQUIREMENTS_PROC* flextvkGetBufferMemoryRequirements = NULL;
PFNVKGETDEVICEMEMORYCOMMITMENT_PROC* flextvkGetDeviceMemoryCommitment = NULL;
PFNVKGETDEVICEPROCADDR_PROC* flextvkGetDeviceProcAddr = NULL;
PFNVKGETDEVICEQUEUE_PROC* flextvkGetDeviceQueue = NULL;
PFNVKGETFENCESTATUS_PROC* flextvkGetFenceStatus = NULL;
PFNVKGETIMAGEMEMORYREQUIREMENTS_PROC* flextvkGetImageMemoryRequirements = NULL;
PFNVKGETIMAGESPARSEMEMORYREQUIREMENTS_PROC* flextvkGetImageSparseMemoryRequirements = NULL;
PFNVKGETIMAGESUBRESOURCELAYOUT_PROC* flextvkGetImageSubresourceLayout = NULL;
PFNVKGETINSTANCEPROCADDR_PROC* flextvkGetInstanceProcAddr = NULL;
PFNVKGETPHYSICALDEVICEFEATURES_PROC* flextvkGetPhysicalDeviceFeatures = NULL;
PFNVKGETPHYSICALDEVICEFORMATPROPERTIES_PROC* flextvkGetPhysicalDeviceFormatProperties = NULL;
PFNVKGETPHYSICALDEVICEIMAGEFORMATPROPERTIES_PROC* flextvkGetPhysicalDeviceImageFormatProperties = NULL;
PFNVKGETPHYSICALDEVICEMEMORYPROPERTIES_PROC* flextvkGetPhysicalDeviceMemoryProperties = NULL;
PFNVKGETPHYSICALDEVICEPROPERTIES_PROC* flextvkGetPhysicalDeviceProperties = NULL;
PFNVKGETPHYSICALDEVICEQUEUEFAMILYPROPERTIES_PROC* flextvkGetPhysicalDeviceQueueFamilyProperties = NULL;
PFNVKGETPHYSICALDEVICESPARSEIMAGEFORMATPROPERTIES_PROC* flextvkGetPhysicalDeviceSparseImageFormatProperties = NULL;
PFNVKGETQUERYPOOLRESULTS_PROC* flextvkGetQueryPoolResults = NULL;
PFNVKINVALIDATEMAPPEDMEMORYRANGES_PROC* flextvkInvalidateMappedMemoryRanges = NULL;
PFNVKMAPMEMORY_PROC* flextvkMapMemory = NULL;
PFNVKQUEUEBINDSPARSE_PROC* flextvkQueueBindSparse = NULL;
PFNVKQUEUESUBMIT_PROC* flextvkQueueSubmit = NULL;
PFNVKQUEUEWAITIDLE_PROC* flextvkQueueWaitIdle = NULL;
PFNVKRESETCOMMANDBUFFER_PROC* flextvkResetCommandBuffer = NULL;
PFNVKRESETCOMMANDPOOL_PROC* flextvkResetCommandPool = NULL;
PFNVKRESETFENCES_PROC* flextvkResetFences = NULL;
PFNVKUNMAPMEMORY_PROC* flextvkUnmapMemory = NULL;
PFNVKWAITFORFENCES_PROC* flextvkWaitForFences = NULL;
PFNVKALLOCATEDESCRIPTORSETS_PROC* flextvkAllocateDescriptorSets = NULL;
PFNVKCMDBINDDESCRIPTORSETS_PROC* flextvkCmdBindDescriptorSets = NULL;
PFNVKCMDBINDPIPELINE_PROC* flextvkCmdBindPipeline = NULL;
PFNVKCMDCLEARCOLORIMAGE_PROC* flextvkCmdClearColorImage = NULL;
PFNVKCMDDISPATCH_PROC* flextvkCmdDispatch = NULL;
PFNVKCMDDISPATCHINDIRECT_PROC* flextvkCmdDispatchIndirect = NULL;
PFNVKCMDPUSHCONSTANTS_PROC* flextvkCmdPushConstants = NULL;
PFNVKCMDRESETEVENT_PROC* flextvkCmdResetEvent = NULL;
PFNVKCMDSETEVENT_PROC* flextvkCmdSetEvent = NULL;
PFNVKCMDWAITEVENTS_PROC* flextvkCmdWaitEvents = NULL;
PFNVKCREATEBUFFERVIEW_PROC* flextvkCreateBufferView = NULL;
PFNVKCREATECOMPUTEPIPELINES_PROC* flextvkCreateComputePipelines = NULL;
PFNVKCREATEDESCRIPTORPOOL_PROC* flextvkCreateDescriptorPool = NULL;
PFNVKCREATEDESCRIPTORSETLAYOUT_PROC* flextvkCreateDescriptorSetLayout = NULL;
PFNVKCREATEEVENT_PROC* flextvkCreateEvent = NULL;
PFNVKCREATEPIPELINECACHE_PROC* flextvkCreatePipelineCache = NULL;
PFNVKCREATEPIPELINELAYOUT_PROC* flextvkCreatePipelineLayout = NULL;
PFNVKCREATESAMPLER_PROC* flextvkCreateSampler = NULL;
PFNVKCREATESHADERMODULE_PROC* flextvkCreateShaderModule = NULL;
PFNVKDESTROYBUFFERVIEW_PROC* flextvkDestroyBufferView = NULL;
PFNVKDESTROYDESCRIPTORPOOL_PROC* flextvkDestroyDescriptorPool = NULL;
PFNVKDESTROYDESCRIPTORSETLAYOUT_PROC* flextvkDestroyDescriptorSetLayout = NULL;
PFNVKDESTROYEVENT_PROC* flextvkDestroyEvent = NULL;
PFNVKDESTROYPIPELINE_PROC* flextvkDestroyPipeline = NULL;
PFNVKDESTROYPIPELINECACHE_PROC* flextvkDestroyPipelineCache = NULL;
PFNVKDESTROYPIPELINELAYOUT_PROC* flextvkDestroyPipelineLayout = NULL;
PFNVKDESTROYSAMPLER_PROC* flextvkDestroySampler = NULL;
PFNVKDESTROYSHADERMODULE_PROC* flextvkDestroyShaderModule = NULL;
PFNVKFREEDESCRIPTORSETS_PROC* flextvkFreeDescriptorSets = NULL;
PFNVKGETEVENTSTATUS_PROC* flextvkGetEventStatus = NULL;
PFNVKGETPIPELINECACHEDATA_PROC* flextvkGetPipelineCacheData = NULL;
PFNVKMERGEPIPELINECACHES_PROC* flextvkMergePipelineCaches = NULL;
PFNVKRESETDESCRIPTORPOOL_PROC* flextvkResetDescriptorPool = NULL;
PFNVKRESETEVENT_PROC* flextvkResetEvent = NULL;
PFNVKSETEVENT_PROC* flextvkSetEvent = NULL;
PFNVKUPDATEDESCRIPTORSETS_PROC* flextvkUpdateDescriptorSets = NULL;
PFNVKCREATEDEBUGREPORTCALLBACKEXT_PROC* flextvkCreateDebugReportCallbackEXT = NULL;
PFNVKDEBUGREPORTMESSAGEEXT_PROC* flextvkDebugReportMessageEXT = NULL;
PFNVKDESTROYDEBUGREPORTCALLBACKEXT_PROC* flextvkDestroyDebugReportCallbackEXT = NULL;
PFNVKCMDBEGINRENDERPASS_PROC* flextvkCmdBeginRenderPass = NULL;
PFNVKCMDBINDINDEXBUFFER_PROC* flextvkCmdBindIndexBuffer = NULL;
PFNVKCMDBINDVERTEXBUFFERS_PROC* flextvkCmdBindVertexBuffers = NULL;
PFNVKCMDBLITIMAGE_PROC* flextvkCmdBlitImage = NULL;
PFNVKCMDCLEARATTACHMENTS_PROC* flextvkCmdClearAttachments = NULL;
PFNVKCMDCLEARDEPTHSTENCILIMAGE_PROC* flextvkCmdClearDepthStencilImage = NULL;
PFNVKCMDDRAW_PROC* flextvkCmdDraw = NULL;
PFNVKCMDDRAWINDEXED_PROC* flextvkCmdDrawIndexed = NULL;
PFNVKCMDDRAWINDEXEDINDIRECT_PROC* flextvkCmdDrawIndexedIndirect = NULL;
PFNVKCMDDRAWINDIRECT_PROC* flextvkCmdDrawIndirect = NULL;
PFNVKCMDENDRENDERPASS_PROC* flextvkCmdEndRenderPass = NULL;
PFNVKCMDNEXTSUBPASS_PROC* flextvkCmdNextSubpass = NULL;
PFNVKCMDRESOLVEIMAGE_PROC* flextvkCmdResolveImage = NULL;
PFNVKCMDSETBLENDCONSTANTS_PROC* flextvkCmdSetBlendConstants = NULL;
PFNVKCMDSETDEPTHBIAS_PROC* flextvkCmdSetDepthBias = NULL;
PFNVKCMDSETDEPTHBOUNDS_PROC* flextvkCmdSetDepthBounds = NULL;
PFNVKCMDSETLINEWIDTH_PROC* flextvkCmdSetLineWidth = NULL;
PFNVKCMDSETSCISSOR_PROC* flextvkCmdSetScissor = NULL;
PFNVKCMDSETSTENCILCOMPAREMASK_PROC* flextvkCmdSetStencilCompareMask = NULL;
PFNVKCMDSETSTENCILREFERENCE_PROC* flextvkCmdSetStencilReference = NULL;
PFNVKCMDSETSTENCILWRITEMASK_PROC* flextvkCmdSetStencilWriteMask = NULL;
PFNVKCMDSETVIEWPORT_PROC* flextvkCmdSetViewport = NULL;
PFNVKCREATEFRAMEBUFFER_PROC* flextvkCreateFramebuffer = NULL;
PFNVKCREATEGRAPHICSPIPELINES_PROC* flextvkCreateGraphicsPipelines = NULL;
PFNVKCREATERENDERPASS_PROC* flextvkCreateRenderPass = NULL;
PFNVKDESTROYFRAMEBUFFER_PROC* flextvkDestroyFramebuffer = NULL;
PFNVKDESTROYRENDERPASS_PROC* flextvkDestroyRenderPass = NULL;
PFNVKGETRENDERAREAGRANULARITY_PROC* flextvkGetRenderAreaGranularity = NULL;
PFNVKTRIMCOMMANDPOOLKHR_PROC* flextvkTrimCommandPoolKHR = NULL;
PFNVKDESTROYSURFACEKHR_PROC* flextvkDestroySurfaceKHR = NULL;
PFNVKGETPHYSICALDEVICESURFACECAPABILITIESKHR_PROC* flextvkGetPhysicalDeviceSurfaceCapabilitiesKHR = NULL;
PFNVKGETPHYSICALDEVICESURFACEFORMATSKHR_PROC* flextvkGetPhysicalDeviceSurfaceFormatsKHR = NULL;
PFNVKGETPHYSICALDEVICESURFACEPRESENTMODESKHR_PROC* flextvkGetPhysicalDeviceSurfacePresentModesKHR = NULL;
PFNVKGETPHYSICALDEVICESURFACESUPPORTKHR_PROC* flextvkGetPhysicalDeviceSurfaceSupportKHR = NULL;
PFNVKACQUIRENEXTIMAGEKHR_PROC* flextvkAcquireNextImageKHR = NULL;
PFNVKCREATESWAPCHAINKHR_PROC* flextvkCreateSwapchainKHR = NULL;
PFNVKDESTROYSWAPCHAINKHR_PROC* flextvkDestroySwapchainKHR = NULL;
PFNVKGETSWAPCHAINIMAGESKHR_PROC* flextvkGetSwapchainImagesKHR = NULL;
PFNVKQUEUEPRESENTKHR_PROC* flextvkQueuePresentKHR = NULL;

static void *vulkanLibrary = NULL;

void flextVkShutdown(void) {
    SDL_UnloadObject(vulkanLibrary);
    vulkanLibrary = NULL;
}

int flextVkInit(void) {
#if defined(__linux__)
    vulkanLibrary = SDL_LoadObject("libvulkan.so");
    if (vulkanLibrary == NULL) {
        vulkanLibrary = SDL_LoadObject("libvulkan.so.1");
    }
#elif defined(__APPLE__)
    vulkanLibrary = SDL_LoadObject("libvulkan.dylib");
#elif defined(_WIN32)
    vulkanLibrary = SDL_LoadObject("vulkan-1.dll");
#else
#error unsupported platform
#endif
    if (vulkanLibrary == NULL) {
        return SDL_SetError("Could not load the vulkan library");
    }
    /* Cast via void* to silence -Wcast-function-type on SDL_FunctionPointer. */
    flextvkCreateInstance = (PFNVKCREATEINSTANCE_PROC*)(void*)SDL_LoadFunction(vulkanLibrary, "vkCreateInstance");
    flextvkEnumerateInstanceExtensionProperties = (PFNVKENUMERATEINSTANCEEXTENSIONPROPERTIES_PROC*)(void*)SDL_LoadFunction(vulkanLibrary, "vkEnumerateInstanceExtensionProperties");
    flextvkEnumerateInstanceLayerProperties = (PFNVKENUMERATEINSTANCELAYERPROPERTIES_PROC*)(void*)SDL_LoadFunction(vulkanLibrary, "vkEnumerateInstanceLayerProperties");
    flextvkGetInstanceProcAddr = (PFNVKGETINSTANCEPROCADDR_PROC*)(void*)SDL_LoadFunction(vulkanLibrary, "vkGetInstanceProcAddr");
    return 0;
}

void flextVkInitInstance(VkInstance instance) {
    /* Cast via void* to silence -Wcast-function-type on PFN_vkVoidFunction. */
    flextvkAllocateCommandBuffers = (PFNVKALLOCATECOMMANDBUFFERS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkAllocateCommandBuffers");
    flextvkAllocateMemory = (PFNVKALLOCATEMEMORY_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkAllocateMemory");
    flextvkBeginCommandBuffer = (PFNVKBEGINCOMMANDBUFFER_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkBeginCommandBuffer");
    flextvkBindBufferMemory = (PFNVKBINDBUFFERMEMORY_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkBindBufferMemory");
    flextvkBindImageMemory = (PFNVKBINDIMAGEMEMORY_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkBindImageMemory");
    flextvkCmdBeginQuery = (PFNVKCMDBEGINQUERY_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdBeginQuery");
    flextvkCmdCopyBuffer = (PFNVKCMDCOPYBUFFER_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdCopyBuffer");
    flextvkCmdCopyBufferToImage = (PFNVKCMDCOPYBUFFERTOIMAGE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdCopyBufferToImage");
    flextvkCmdCopyImage = (PFNVKCMDCOPYIMAGE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdCopyImage");
    flextvkCmdCopyImageToBuffer = (PFNVKCMDCOPYIMAGETOBUFFER_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdCopyImageToBuffer");
    flextvkCmdCopyQueryPoolResults = (PFNVKCMDCOPYQUERYPOOLRESULTS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdCopyQueryPoolResults");
    flextvkCmdEndQuery = (PFNVKCMDENDQUERY_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdEndQuery");
    flextvkCmdExecuteCommands = (PFNVKCMDEXECUTECOMMANDS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdExecuteCommands");
    flextvkCmdFillBuffer = (PFNVKCMDFILLBUFFER_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdFillBuffer");
    flextvkCmdPipelineBarrier = (PFNVKCMDPIPELINEBARRIER_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdPipelineBarrier");
    flextvkCmdResetQueryPool = (PFNVKCMDRESETQUERYPOOL_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdResetQueryPool");
    flextvkCmdUpdateBuffer = (PFNVKCMDUPDATEBUFFER_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdUpdateBuffer");
    flextvkCmdWriteTimestamp = (PFNVKCMDWRITETIMESTAMP_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdWriteTimestamp");
    flextvkCreateBuffer = (PFNVKCREATEBUFFER_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateBuffer");
    flextvkCreateCommandPool = (PFNVKCREATECOMMANDPOOL_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateCommandPool");
    flextvkCreateDevice = (PFNVKCREATEDEVICE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateDevice");
    flextvkCreateFence = (PFNVKCREATEFENCE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateFence");
    flextvkCreateImage = (PFNVKCREATEIMAGE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateImage");
    flextvkCreateImageView = (PFNVKCREATEIMAGEVIEW_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateImageView");
    flextvkCreateQueryPool = (PFNVKCREATEQUERYPOOL_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateQueryPool");
    flextvkCreateSemaphore = (PFNVKCREATESEMAPHORE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateSemaphore");
    flextvkDestroyBuffer = (PFNVKDESTROYBUFFER_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyBuffer");
    flextvkDestroyCommandPool = (PFNVKDESTROYCOMMANDPOOL_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyCommandPool");
    flextvkDestroyDevice = (PFNVKDESTROYDEVICE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyDevice");
    flextvkDestroyFence = (PFNVKDESTROYFENCE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyFence");
    flextvkDestroyImage = (PFNVKDESTROYIMAGE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyImage");
    flextvkDestroyImageView = (PFNVKDESTROYIMAGEVIEW_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyImageView");
    flextvkDestroyInstance = (PFNVKDESTROYINSTANCE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyInstance");
    flextvkDestroyQueryPool = (PFNVKDESTROYQUERYPOOL_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyQueryPool");
    flextvkDestroySemaphore = (PFNVKDESTROYSEMAPHORE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroySemaphore");
    flextvkDeviceWaitIdle = (PFNVKDEVICEWAITIDLE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDeviceWaitIdle");
    flextvkEndCommandBuffer = (PFNVKENDCOMMANDBUFFER_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkEndCommandBuffer");
    flextvkEnumerateDeviceExtensionProperties = (PFNVKENUMERATEDEVICEEXTENSIONPROPERTIES_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties");
    flextvkEnumerateDeviceLayerProperties = (PFNVKENUMERATEDEVICELAYERPROPERTIES_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkEnumerateDeviceLayerProperties");
    flextvkEnumeratePhysicalDevices = (PFNVKENUMERATEPHYSICALDEVICES_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
    flextvkFlushMappedMemoryRanges = (PFNVKFLUSHMAPPEDMEMORYRANGES_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkFlushMappedMemoryRanges");
    flextvkFreeCommandBuffers = (PFNVKFREECOMMANDBUFFERS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkFreeCommandBuffers");
    flextvkFreeMemory = (PFNVKFREEMEMORY_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkFreeMemory");
    flextvkGetBufferMemoryRequirements = (PFNVKGETBUFFERMEMORYREQUIREMENTS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetBufferMemoryRequirements");
    flextvkGetDeviceMemoryCommitment = (PFNVKGETDEVICEMEMORYCOMMITMENT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetDeviceMemoryCommitment");
    flextvkGetDeviceProcAddr = (PFNVKGETDEVICEPROCADDR_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr");
    flextvkGetDeviceQueue = (PFNVKGETDEVICEQUEUE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetDeviceQueue");
    flextvkGetFenceStatus = (PFNVKGETFENCESTATUS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetFenceStatus");
    flextvkGetImageMemoryRequirements = (PFNVKGETIMAGEMEMORYREQUIREMENTS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetImageMemoryRequirements");
    flextvkGetImageSparseMemoryRequirements = (PFNVKGETIMAGESPARSEMEMORYREQUIREMENTS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetImageSparseMemoryRequirements");
    flextvkGetImageSubresourceLayout = (PFNVKGETIMAGESUBRESOURCELAYOUT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetImageSubresourceLayout");
    flextvkGetPhysicalDeviceFeatures = (PFNVKGETPHYSICALDEVICEFEATURES_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures");
    flextvkGetPhysicalDeviceFormatProperties = (PFNVKGETPHYSICALDEVICEFORMATPROPERTIES_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFormatProperties");
    flextvkGetPhysicalDeviceImageFormatProperties = (PFNVKGETPHYSICALDEVICEIMAGEFORMATPROPERTIES_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceImageFormatProperties");
    flextvkGetPhysicalDeviceMemoryProperties = (PFNVKGETPHYSICALDEVICEMEMORYPROPERTIES_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties");
    flextvkGetPhysicalDeviceProperties = (PFNVKGETPHYSICALDEVICEPROPERTIES_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties");
    flextvkGetPhysicalDeviceQueueFamilyProperties = (PFNVKGETPHYSICALDEVICEQUEUEFAMILYPROPERTIES_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties");
    flextvkGetPhysicalDeviceSparseImageFormatProperties = (PFNVKGETPHYSICALDEVICESPARSEIMAGEFORMATPROPERTIES_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSparseImageFormatProperties");
    flextvkGetQueryPoolResults = (PFNVKGETQUERYPOOLRESULTS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetQueryPoolResults");
    flextvkInvalidateMappedMemoryRanges = (PFNVKINVALIDATEMAPPEDMEMORYRANGES_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkInvalidateMappedMemoryRanges");
    flextvkMapMemory = (PFNVKMAPMEMORY_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkMapMemory");
    flextvkQueueBindSparse = (PFNVKQUEUEBINDSPARSE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkQueueBindSparse");
    flextvkQueueSubmit = (PFNVKQUEUESUBMIT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkQueueSubmit");
    flextvkQueueWaitIdle = (PFNVKQUEUEWAITIDLE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkQueueWaitIdle");
    flextvkResetCommandBuffer = (PFNVKRESETCOMMANDBUFFER_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkResetCommandBuffer");
    flextvkResetCommandPool = (PFNVKRESETCOMMANDPOOL_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkResetCommandPool");
    flextvkResetFences = (PFNVKRESETFENCES_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkResetFences");
    flextvkUnmapMemory = (PFNVKUNMAPMEMORY_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkUnmapMemory");
    flextvkWaitForFences = (PFNVKWAITFORFENCES_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkWaitForFences");
    flextvkAllocateDescriptorSets = (PFNVKALLOCATEDESCRIPTORSETS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkAllocateDescriptorSets");
    flextvkCmdBindDescriptorSets = (PFNVKCMDBINDDESCRIPTORSETS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdBindDescriptorSets");
    flextvkCmdBindPipeline = (PFNVKCMDBINDPIPELINE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdBindPipeline");
    flextvkCmdClearColorImage = (PFNVKCMDCLEARCOLORIMAGE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdClearColorImage");
    flextvkCmdDispatch = (PFNVKCMDDISPATCH_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdDispatch");
    flextvkCmdDispatchIndirect = (PFNVKCMDDISPATCHINDIRECT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdDispatchIndirect");
    flextvkCmdPushConstants = (PFNVKCMDPUSHCONSTANTS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdPushConstants");
    flextvkCmdResetEvent = (PFNVKCMDRESETEVENT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdResetEvent");
    flextvkCmdSetEvent = (PFNVKCMDSETEVENT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdSetEvent");
    flextvkCmdWaitEvents = (PFNVKCMDWAITEVENTS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdWaitEvents");
    flextvkCreateBufferView = (PFNVKCREATEBUFFERVIEW_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateBufferView");
    flextvkCreateComputePipelines = (PFNVKCREATECOMPUTEPIPELINES_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateComputePipelines");
    flextvkCreateDescriptorPool = (PFNVKCREATEDESCRIPTORPOOL_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateDescriptorPool");
    flextvkCreateDescriptorSetLayout = (PFNVKCREATEDESCRIPTORSETLAYOUT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateDescriptorSetLayout");
    flextvkCreateEvent = (PFNVKCREATEEVENT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateEvent");
    flextvkCreatePipelineCache = (PFNVKCREATEPIPELINECACHE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreatePipelineCache");
    flextvkCreatePipelineLayout = (PFNVKCREATEPIPELINELAYOUT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreatePipelineLayout");
    flextvkCreateSampler = (PFNVKCREATESAMPLER_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateSampler");
    flextvkCreateShaderModule = (PFNVKCREATESHADERMODULE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateShaderModule");
    flextvkDestroyBufferView = (PFNVKDESTROYBUFFERVIEW_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyBufferView");
    flextvkDestroyDescriptorPool = (PFNVKDESTROYDESCRIPTORPOOL_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyDescriptorPool");
    flextvkDestroyDescriptorSetLayout = (PFNVKDESTROYDESCRIPTORSETLAYOUT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyDescriptorSetLayout");
    flextvkDestroyEvent = (PFNVKDESTROYEVENT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyEvent");
    flextvkDestroyPipeline = (PFNVKDESTROYPIPELINE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyPipeline");
    flextvkDestroyPipelineCache = (PFNVKDESTROYPIPELINECACHE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyPipelineCache");
    flextvkDestroyPipelineLayout = (PFNVKDESTROYPIPELINELAYOUT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyPipelineLayout");
    flextvkDestroySampler = (PFNVKDESTROYSAMPLER_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroySampler");
    flextvkDestroyShaderModule = (PFNVKDESTROYSHADERMODULE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyShaderModule");
    flextvkFreeDescriptorSets = (PFNVKFREEDESCRIPTORSETS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkFreeDescriptorSets");
    flextvkGetEventStatus = (PFNVKGETEVENTSTATUS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetEventStatus");
    flextvkGetPipelineCacheData = (PFNVKGETPIPELINECACHEDATA_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetPipelineCacheData");
    flextvkMergePipelineCaches = (PFNVKMERGEPIPELINECACHES_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkMergePipelineCaches");
    flextvkResetDescriptorPool = (PFNVKRESETDESCRIPTORPOOL_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkResetDescriptorPool");
    flextvkResetEvent = (PFNVKRESETEVENT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkResetEvent");
    flextvkSetEvent = (PFNVKSETEVENT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkSetEvent");
    flextvkUpdateDescriptorSets = (PFNVKUPDATEDESCRIPTORSETS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkUpdateDescriptorSets");
    flextvkCreateDebugReportCallbackEXT = (PFNVKCREATEDEBUGREPORTCALLBACKEXT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    flextvkDebugReportMessageEXT = (PFNVKDEBUGREPORTMESSAGEEXT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDebugReportMessageEXT");
    flextvkDestroyDebugReportCallbackEXT = (PFNVKDESTROYDEBUGREPORTCALLBACKEXT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    flextvkCmdBeginRenderPass = (PFNVKCMDBEGINRENDERPASS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdBeginRenderPass");
    flextvkCmdBindIndexBuffer = (PFNVKCMDBINDINDEXBUFFER_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdBindIndexBuffer");
    flextvkCmdBindVertexBuffers = (PFNVKCMDBINDVERTEXBUFFERS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdBindVertexBuffers");
    flextvkCmdBlitImage = (PFNVKCMDBLITIMAGE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdBlitImage");
    flextvkCmdClearAttachments = (PFNVKCMDCLEARATTACHMENTS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdClearAttachments");
    flextvkCmdClearDepthStencilImage = (PFNVKCMDCLEARDEPTHSTENCILIMAGE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdClearDepthStencilImage");
    flextvkCmdDraw = (PFNVKCMDDRAW_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdDraw");
    flextvkCmdDrawIndexed = (PFNVKCMDDRAWINDEXED_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdDrawIndexed");
    flextvkCmdDrawIndexedIndirect = (PFNVKCMDDRAWINDEXEDINDIRECT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdDrawIndexedIndirect");
    flextvkCmdDrawIndirect = (PFNVKCMDDRAWINDIRECT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdDrawIndirect");
    flextvkCmdEndRenderPass = (PFNVKCMDENDRENDERPASS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdEndRenderPass");
    flextvkCmdNextSubpass = (PFNVKCMDNEXTSUBPASS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdNextSubpass");
    flextvkCmdResolveImage = (PFNVKCMDRESOLVEIMAGE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdResolveImage");
    flextvkCmdSetBlendConstants = (PFNVKCMDSETBLENDCONSTANTS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdSetBlendConstants");
    flextvkCmdSetDepthBias = (PFNVKCMDSETDEPTHBIAS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdSetDepthBias");
    flextvkCmdSetDepthBounds = (PFNVKCMDSETDEPTHBOUNDS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdSetDepthBounds");
    flextvkCmdSetLineWidth = (PFNVKCMDSETLINEWIDTH_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdSetLineWidth");
    flextvkCmdSetScissor = (PFNVKCMDSETSCISSOR_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdSetScissor");
    flextvkCmdSetStencilCompareMask = (PFNVKCMDSETSTENCILCOMPAREMASK_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdSetStencilCompareMask");
    flextvkCmdSetStencilReference = (PFNVKCMDSETSTENCILREFERENCE_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdSetStencilReference");
    flextvkCmdSetStencilWriteMask = (PFNVKCMDSETSTENCILWRITEMASK_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdSetStencilWriteMask");
    flextvkCmdSetViewport = (PFNVKCMDSETVIEWPORT_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCmdSetViewport");
    flextvkCreateFramebuffer = (PFNVKCREATEFRAMEBUFFER_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateFramebuffer");
    flextvkCreateGraphicsPipelines = (PFNVKCREATEGRAPHICSPIPELINES_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateGraphicsPipelines");
    flextvkCreateRenderPass = (PFNVKCREATERENDERPASS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateRenderPass");
    flextvkDestroyFramebuffer = (PFNVKDESTROYFRAMEBUFFER_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyFramebuffer");
    flextvkDestroyRenderPass = (PFNVKDESTROYRENDERPASS_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroyRenderPass");
    flextvkGetRenderAreaGranularity = (PFNVKGETRENDERAREAGRANULARITY_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetRenderAreaGranularity");
    flextvkTrimCommandPoolKHR = (PFNVKTRIMCOMMANDPOOLKHR_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkTrimCommandPoolKHR");
    flextvkDestroySurfaceKHR = (PFNVKDESTROYSURFACEKHR_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroySurfaceKHR");
    flextvkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFNVKGETPHYSICALDEVICESURFACECAPABILITIESKHR_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    flextvkGetPhysicalDeviceSurfaceFormatsKHR = (PFNVKGETPHYSICALDEVICESURFACEFORMATSKHR_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    flextvkGetPhysicalDeviceSurfacePresentModesKHR = (PFNVKGETPHYSICALDEVICESURFACEPRESENTMODESKHR_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
    flextvkGetPhysicalDeviceSurfaceSupportKHR = (PFNVKGETPHYSICALDEVICESURFACESUPPORTKHR_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
    flextvkAcquireNextImageKHR = (PFNVKACQUIRENEXTIMAGEKHR_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkAcquireNextImageKHR");
    flextvkCreateSwapchainKHR = (PFNVKCREATESWAPCHAINKHR_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkCreateSwapchainKHR");
    flextvkDestroySwapchainKHR = (PFNVKDESTROYSWAPCHAINKHR_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkDestroySwapchainKHR");
    flextvkGetSwapchainImagesKHR = (PFNVKGETSWAPCHAINIMAGESKHR_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkGetSwapchainImagesKHR");
    flextvkQueuePresentKHR = (PFNVKQUEUEPRESENTKHR_PROC*)(void*)flextvkGetInstanceProcAddr(instance, "vkQueuePresentKHR");
}

#ifdef __cplusplus
}
#endif
