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
    flextvkCreateInstance = (PFNVKCREATEINSTANCE_PROC*)SDL_LoadFunction(vulkanLibrary, "vkCreateInstance");
    flextvkEnumerateInstanceExtensionProperties = (PFNVKENUMERATEINSTANCEEXTENSIONPROPERTIES_PROC*)SDL_LoadFunction(vulkanLibrary, "vkEnumerateInstanceExtensionProperties");
    flextvkEnumerateInstanceLayerProperties = (PFNVKENUMERATEINSTANCELAYERPROPERTIES_PROC*)SDL_LoadFunction(vulkanLibrary, "vkEnumerateInstanceLayerProperties");
    flextvkGetInstanceProcAddr = (PFNVKGETINSTANCEPROCADDR_PROC*)SDL_LoadFunction(vulkanLibrary, "vkGetInstanceProcAddr");
    return 0;
}

void flextVkInitInstance(VkInstance instance) {
    flextvkAllocateCommandBuffers = (PFNVKALLOCATECOMMANDBUFFERS_PROC*)flextvkGetInstanceProcAddr(instance, "vkAllocateCommandBuffers");
    flextvkAllocateMemory = (PFNVKALLOCATEMEMORY_PROC*)flextvkGetInstanceProcAddr(instance, "vkAllocateMemory");
    flextvkBeginCommandBuffer = (PFNVKBEGINCOMMANDBUFFER_PROC*)flextvkGetInstanceProcAddr(instance, "vkBeginCommandBuffer");
    flextvkBindBufferMemory = (PFNVKBINDBUFFERMEMORY_PROC*)flextvkGetInstanceProcAddr(instance, "vkBindBufferMemory");
    flextvkBindImageMemory = (PFNVKBINDIMAGEMEMORY_PROC*)flextvkGetInstanceProcAddr(instance, "vkBindImageMemory");
    flextvkCmdBeginQuery = (PFNVKCMDBEGINQUERY_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdBeginQuery");
    flextvkCmdCopyBuffer = (PFNVKCMDCOPYBUFFER_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdCopyBuffer");
    flextvkCmdCopyBufferToImage = (PFNVKCMDCOPYBUFFERTOIMAGE_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdCopyBufferToImage");
    flextvkCmdCopyImage = (PFNVKCMDCOPYIMAGE_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdCopyImage");
    flextvkCmdCopyImageToBuffer = (PFNVKCMDCOPYIMAGETOBUFFER_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdCopyImageToBuffer");
    flextvkCmdCopyQueryPoolResults = (PFNVKCMDCOPYQUERYPOOLRESULTS_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdCopyQueryPoolResults");
    flextvkCmdEndQuery = (PFNVKCMDENDQUERY_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdEndQuery");
    flextvkCmdExecuteCommands = (PFNVKCMDEXECUTECOMMANDS_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdExecuteCommands");
    flextvkCmdFillBuffer = (PFNVKCMDFILLBUFFER_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdFillBuffer");
    flextvkCmdPipelineBarrier = (PFNVKCMDPIPELINEBARRIER_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdPipelineBarrier");
    flextvkCmdResetQueryPool = (PFNVKCMDRESETQUERYPOOL_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdResetQueryPool");
    flextvkCmdUpdateBuffer = (PFNVKCMDUPDATEBUFFER_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdUpdateBuffer");
    flextvkCmdWriteTimestamp = (PFNVKCMDWRITETIMESTAMP_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdWriteTimestamp");
    flextvkCreateBuffer = (PFNVKCREATEBUFFER_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateBuffer");
    flextvkCreateCommandPool = (PFNVKCREATECOMMANDPOOL_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateCommandPool");
    flextvkCreateDevice = (PFNVKCREATEDEVICE_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateDevice");
    flextvkCreateFence = (PFNVKCREATEFENCE_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateFence");
    flextvkCreateImage = (PFNVKCREATEIMAGE_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateImage");
    flextvkCreateImageView = (PFNVKCREATEIMAGEVIEW_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateImageView");
    flextvkCreateQueryPool = (PFNVKCREATEQUERYPOOL_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateQueryPool");
    flextvkCreateSemaphore = (PFNVKCREATESEMAPHORE_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateSemaphore");
    flextvkDestroyBuffer = (PFNVKDESTROYBUFFER_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyBuffer");
    flextvkDestroyCommandPool = (PFNVKDESTROYCOMMANDPOOL_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyCommandPool");
    flextvkDestroyDevice = (PFNVKDESTROYDEVICE_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyDevice");
    flextvkDestroyFence = (PFNVKDESTROYFENCE_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyFence");
    flextvkDestroyImage = (PFNVKDESTROYIMAGE_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyImage");
    flextvkDestroyImageView = (PFNVKDESTROYIMAGEVIEW_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyImageView");
    flextvkDestroyInstance = (PFNVKDESTROYINSTANCE_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyInstance");
    flextvkDestroyQueryPool = (PFNVKDESTROYQUERYPOOL_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyQueryPool");
    flextvkDestroySemaphore = (PFNVKDESTROYSEMAPHORE_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroySemaphore");
    flextvkDeviceWaitIdle = (PFNVKDEVICEWAITIDLE_PROC*)flextvkGetInstanceProcAddr(instance, "vkDeviceWaitIdle");
    flextvkEndCommandBuffer = (PFNVKENDCOMMANDBUFFER_PROC*)flextvkGetInstanceProcAddr(instance, "vkEndCommandBuffer");
    flextvkEnumerateDeviceExtensionProperties = (PFNVKENUMERATEDEVICEEXTENSIONPROPERTIES_PROC*)flextvkGetInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties");
    flextvkEnumerateDeviceLayerProperties = (PFNVKENUMERATEDEVICELAYERPROPERTIES_PROC*)flextvkGetInstanceProcAddr(instance, "vkEnumerateDeviceLayerProperties");
    flextvkEnumeratePhysicalDevices = (PFNVKENUMERATEPHYSICALDEVICES_PROC*)flextvkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
    flextvkFlushMappedMemoryRanges = (PFNVKFLUSHMAPPEDMEMORYRANGES_PROC*)flextvkGetInstanceProcAddr(instance, "vkFlushMappedMemoryRanges");
    flextvkFreeCommandBuffers = (PFNVKFREECOMMANDBUFFERS_PROC*)flextvkGetInstanceProcAddr(instance, "vkFreeCommandBuffers");
    flextvkFreeMemory = (PFNVKFREEMEMORY_PROC*)flextvkGetInstanceProcAddr(instance, "vkFreeMemory");
    flextvkGetBufferMemoryRequirements = (PFNVKGETBUFFERMEMORYREQUIREMENTS_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetBufferMemoryRequirements");
    flextvkGetDeviceMemoryCommitment = (PFNVKGETDEVICEMEMORYCOMMITMENT_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetDeviceMemoryCommitment");
    flextvkGetDeviceProcAddr = (PFNVKGETDEVICEPROCADDR_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr");
    flextvkGetDeviceQueue = (PFNVKGETDEVICEQUEUE_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetDeviceQueue");
    flextvkGetFenceStatus = (PFNVKGETFENCESTATUS_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetFenceStatus");
    flextvkGetImageMemoryRequirements = (PFNVKGETIMAGEMEMORYREQUIREMENTS_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetImageMemoryRequirements");
    flextvkGetImageSparseMemoryRequirements = (PFNVKGETIMAGESPARSEMEMORYREQUIREMENTS_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetImageSparseMemoryRequirements");
    flextvkGetImageSubresourceLayout = (PFNVKGETIMAGESUBRESOURCELAYOUT_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetImageSubresourceLayout");
    flextvkGetPhysicalDeviceFeatures = (PFNVKGETPHYSICALDEVICEFEATURES_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures");
    flextvkGetPhysicalDeviceFormatProperties = (PFNVKGETPHYSICALDEVICEFORMATPROPERTIES_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFormatProperties");
    flextvkGetPhysicalDeviceImageFormatProperties = (PFNVKGETPHYSICALDEVICEIMAGEFORMATPROPERTIES_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceImageFormatProperties");
    flextvkGetPhysicalDeviceMemoryProperties = (PFNVKGETPHYSICALDEVICEMEMORYPROPERTIES_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties");
    flextvkGetPhysicalDeviceProperties = (PFNVKGETPHYSICALDEVICEPROPERTIES_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties");
    flextvkGetPhysicalDeviceQueueFamilyProperties = (PFNVKGETPHYSICALDEVICEQUEUEFAMILYPROPERTIES_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties");
    flextvkGetPhysicalDeviceSparseImageFormatProperties = (PFNVKGETPHYSICALDEVICESPARSEIMAGEFORMATPROPERTIES_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSparseImageFormatProperties");
    flextvkGetQueryPoolResults = (PFNVKGETQUERYPOOLRESULTS_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetQueryPoolResults");
    flextvkInvalidateMappedMemoryRanges = (PFNVKINVALIDATEMAPPEDMEMORYRANGES_PROC*)flextvkGetInstanceProcAddr(instance, "vkInvalidateMappedMemoryRanges");
    flextvkMapMemory = (PFNVKMAPMEMORY_PROC*)flextvkGetInstanceProcAddr(instance, "vkMapMemory");
    flextvkQueueBindSparse = (PFNVKQUEUEBINDSPARSE_PROC*)flextvkGetInstanceProcAddr(instance, "vkQueueBindSparse");
    flextvkQueueSubmit = (PFNVKQUEUESUBMIT_PROC*)flextvkGetInstanceProcAddr(instance, "vkQueueSubmit");
    flextvkQueueWaitIdle = (PFNVKQUEUEWAITIDLE_PROC*)flextvkGetInstanceProcAddr(instance, "vkQueueWaitIdle");
    flextvkResetCommandBuffer = (PFNVKRESETCOMMANDBUFFER_PROC*)flextvkGetInstanceProcAddr(instance, "vkResetCommandBuffer");
    flextvkResetCommandPool = (PFNVKRESETCOMMANDPOOL_PROC*)flextvkGetInstanceProcAddr(instance, "vkResetCommandPool");
    flextvkResetFences = (PFNVKRESETFENCES_PROC*)flextvkGetInstanceProcAddr(instance, "vkResetFences");
    flextvkUnmapMemory = (PFNVKUNMAPMEMORY_PROC*)flextvkGetInstanceProcAddr(instance, "vkUnmapMemory");
    flextvkWaitForFences = (PFNVKWAITFORFENCES_PROC*)flextvkGetInstanceProcAddr(instance, "vkWaitForFences");
    flextvkAllocateDescriptorSets = (PFNVKALLOCATEDESCRIPTORSETS_PROC*)flextvkGetInstanceProcAddr(instance, "vkAllocateDescriptorSets");
    flextvkCmdBindDescriptorSets = (PFNVKCMDBINDDESCRIPTORSETS_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdBindDescriptorSets");
    flextvkCmdBindPipeline = (PFNVKCMDBINDPIPELINE_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdBindPipeline");
    flextvkCmdClearColorImage = (PFNVKCMDCLEARCOLORIMAGE_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdClearColorImage");
    flextvkCmdDispatch = (PFNVKCMDDISPATCH_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdDispatch");
    flextvkCmdDispatchIndirect = (PFNVKCMDDISPATCHINDIRECT_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdDispatchIndirect");
    flextvkCmdPushConstants = (PFNVKCMDPUSHCONSTANTS_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdPushConstants");
    flextvkCmdResetEvent = (PFNVKCMDRESETEVENT_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdResetEvent");
    flextvkCmdSetEvent = (PFNVKCMDSETEVENT_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdSetEvent");
    flextvkCmdWaitEvents = (PFNVKCMDWAITEVENTS_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdWaitEvents");
    flextvkCreateBufferView = (PFNVKCREATEBUFFERVIEW_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateBufferView");
    flextvkCreateComputePipelines = (PFNVKCREATECOMPUTEPIPELINES_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateComputePipelines");
    flextvkCreateDescriptorPool = (PFNVKCREATEDESCRIPTORPOOL_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateDescriptorPool");
    flextvkCreateDescriptorSetLayout = (PFNVKCREATEDESCRIPTORSETLAYOUT_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateDescriptorSetLayout");
    flextvkCreateEvent = (PFNVKCREATEEVENT_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateEvent");
    flextvkCreatePipelineCache = (PFNVKCREATEPIPELINECACHE_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreatePipelineCache");
    flextvkCreatePipelineLayout = (PFNVKCREATEPIPELINELAYOUT_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreatePipelineLayout");
    flextvkCreateSampler = (PFNVKCREATESAMPLER_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateSampler");
    flextvkCreateShaderModule = (PFNVKCREATESHADERMODULE_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateShaderModule");
    flextvkDestroyBufferView = (PFNVKDESTROYBUFFERVIEW_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyBufferView");
    flextvkDestroyDescriptorPool = (PFNVKDESTROYDESCRIPTORPOOL_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyDescriptorPool");
    flextvkDestroyDescriptorSetLayout = (PFNVKDESTROYDESCRIPTORSETLAYOUT_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyDescriptorSetLayout");
    flextvkDestroyEvent = (PFNVKDESTROYEVENT_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyEvent");
    flextvkDestroyPipeline = (PFNVKDESTROYPIPELINE_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyPipeline");
    flextvkDestroyPipelineCache = (PFNVKDESTROYPIPELINECACHE_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyPipelineCache");
    flextvkDestroyPipelineLayout = (PFNVKDESTROYPIPELINELAYOUT_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyPipelineLayout");
    flextvkDestroySampler = (PFNVKDESTROYSAMPLER_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroySampler");
    flextvkDestroyShaderModule = (PFNVKDESTROYSHADERMODULE_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyShaderModule");
    flextvkFreeDescriptorSets = (PFNVKFREEDESCRIPTORSETS_PROC*)flextvkGetInstanceProcAddr(instance, "vkFreeDescriptorSets");
    flextvkGetEventStatus = (PFNVKGETEVENTSTATUS_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetEventStatus");
    flextvkGetPipelineCacheData = (PFNVKGETPIPELINECACHEDATA_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetPipelineCacheData");
    flextvkMergePipelineCaches = (PFNVKMERGEPIPELINECACHES_PROC*)flextvkGetInstanceProcAddr(instance, "vkMergePipelineCaches");
    flextvkResetDescriptorPool = (PFNVKRESETDESCRIPTORPOOL_PROC*)flextvkGetInstanceProcAddr(instance, "vkResetDescriptorPool");
    flextvkResetEvent = (PFNVKRESETEVENT_PROC*)flextvkGetInstanceProcAddr(instance, "vkResetEvent");
    flextvkSetEvent = (PFNVKSETEVENT_PROC*)flextvkGetInstanceProcAddr(instance, "vkSetEvent");
    flextvkUpdateDescriptorSets = (PFNVKUPDATEDESCRIPTORSETS_PROC*)flextvkGetInstanceProcAddr(instance, "vkUpdateDescriptorSets");
    flextvkCreateDebugReportCallbackEXT = (PFNVKCREATEDEBUGREPORTCALLBACKEXT_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    flextvkDebugReportMessageEXT = (PFNVKDEBUGREPORTMESSAGEEXT_PROC*)flextvkGetInstanceProcAddr(instance, "vkDebugReportMessageEXT");
    flextvkDestroyDebugReportCallbackEXT = (PFNVKDESTROYDEBUGREPORTCALLBACKEXT_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    flextvkCmdBeginRenderPass = (PFNVKCMDBEGINRENDERPASS_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdBeginRenderPass");
    flextvkCmdBindIndexBuffer = (PFNVKCMDBINDINDEXBUFFER_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdBindIndexBuffer");
    flextvkCmdBindVertexBuffers = (PFNVKCMDBINDVERTEXBUFFERS_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdBindVertexBuffers");
    flextvkCmdBlitImage = (PFNVKCMDBLITIMAGE_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdBlitImage");
    flextvkCmdClearAttachments = (PFNVKCMDCLEARATTACHMENTS_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdClearAttachments");
    flextvkCmdClearDepthStencilImage = (PFNVKCMDCLEARDEPTHSTENCILIMAGE_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdClearDepthStencilImage");
    flextvkCmdDraw = (PFNVKCMDDRAW_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdDraw");
    flextvkCmdDrawIndexed = (PFNVKCMDDRAWINDEXED_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdDrawIndexed");
    flextvkCmdDrawIndexedIndirect = (PFNVKCMDDRAWINDEXEDINDIRECT_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdDrawIndexedIndirect");
    flextvkCmdDrawIndirect = (PFNVKCMDDRAWINDIRECT_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdDrawIndirect");
    flextvkCmdEndRenderPass = (PFNVKCMDENDRENDERPASS_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdEndRenderPass");
    flextvkCmdNextSubpass = (PFNVKCMDNEXTSUBPASS_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdNextSubpass");
    flextvkCmdResolveImage = (PFNVKCMDRESOLVEIMAGE_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdResolveImage");
    flextvkCmdSetBlendConstants = (PFNVKCMDSETBLENDCONSTANTS_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdSetBlendConstants");
    flextvkCmdSetDepthBias = (PFNVKCMDSETDEPTHBIAS_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdSetDepthBias");
    flextvkCmdSetDepthBounds = (PFNVKCMDSETDEPTHBOUNDS_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdSetDepthBounds");
    flextvkCmdSetLineWidth = (PFNVKCMDSETLINEWIDTH_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdSetLineWidth");
    flextvkCmdSetScissor = (PFNVKCMDSETSCISSOR_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdSetScissor");
    flextvkCmdSetStencilCompareMask = (PFNVKCMDSETSTENCILCOMPAREMASK_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdSetStencilCompareMask");
    flextvkCmdSetStencilReference = (PFNVKCMDSETSTENCILREFERENCE_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdSetStencilReference");
    flextvkCmdSetStencilWriteMask = (PFNVKCMDSETSTENCILWRITEMASK_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdSetStencilWriteMask");
    flextvkCmdSetViewport = (PFNVKCMDSETVIEWPORT_PROC*)flextvkGetInstanceProcAddr(instance, "vkCmdSetViewport");
    flextvkCreateFramebuffer = (PFNVKCREATEFRAMEBUFFER_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateFramebuffer");
    flextvkCreateGraphicsPipelines = (PFNVKCREATEGRAPHICSPIPELINES_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateGraphicsPipelines");
    flextvkCreateRenderPass = (PFNVKCREATERENDERPASS_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateRenderPass");
    flextvkDestroyFramebuffer = (PFNVKDESTROYFRAMEBUFFER_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyFramebuffer");
    flextvkDestroyRenderPass = (PFNVKDESTROYRENDERPASS_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroyRenderPass");
    flextvkGetRenderAreaGranularity = (PFNVKGETRENDERAREAGRANULARITY_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetRenderAreaGranularity");
    flextvkTrimCommandPoolKHR = (PFNVKTRIMCOMMANDPOOLKHR_PROC*)flextvkGetInstanceProcAddr(instance, "vkTrimCommandPoolKHR");
    flextvkDestroySurfaceKHR = (PFNVKDESTROYSURFACEKHR_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroySurfaceKHR");
    flextvkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFNVKGETPHYSICALDEVICESURFACECAPABILITIESKHR_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    flextvkGetPhysicalDeviceSurfaceFormatsKHR = (PFNVKGETPHYSICALDEVICESURFACEFORMATSKHR_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    flextvkGetPhysicalDeviceSurfacePresentModesKHR = (PFNVKGETPHYSICALDEVICESURFACEPRESENTMODESKHR_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
    flextvkGetPhysicalDeviceSurfaceSupportKHR = (PFNVKGETPHYSICALDEVICESURFACESUPPORTKHR_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
    flextvkAcquireNextImageKHR = (PFNVKACQUIRENEXTIMAGEKHR_PROC*)flextvkGetInstanceProcAddr(instance, "vkAcquireNextImageKHR");
    flextvkCreateSwapchainKHR = (PFNVKCREATESWAPCHAINKHR_PROC*)flextvkGetInstanceProcAddr(instance, "vkCreateSwapchainKHR");
    flextvkDestroySwapchainKHR = (PFNVKDESTROYSWAPCHAINKHR_PROC*)flextvkGetInstanceProcAddr(instance, "vkDestroySwapchainKHR");
    flextvkGetSwapchainImagesKHR = (PFNVKGETSWAPCHAINIMAGESKHR_PROC*)flextvkGetInstanceProcAddr(instance, "vkGetSwapchainImagesKHR");
    flextvkQueuePresentKHR = (PFNVKQUEUEPRESENTKHR_PROC*)flextvkGetInstanceProcAddr(instance, "vkQueuePresentKHR");
}

#ifdef __cplusplus
}
#endif
