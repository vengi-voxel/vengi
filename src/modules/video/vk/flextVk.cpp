/*
    This file was generated using https://github.com/mosra/flextgl:

        path/to/flextGLgen.py -T vulkan-dynamic -D /home/mgerhardy/dev/engine/src/modules/video/vk profiles/vulkan.txt

    Do not edit directly, modify the template or profile and regenerate.
*/

#include "flextVk.h"

/* The following definitions and flextDynamicLoader are borrowed/modified from vulkan.hpp DynamicLoader */
// Copyright (c) 2015-2020 The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT
//
#if defined(__linux__) || defined(__APPLE__)
    #include <dlfcn.h>
#elif defined(_WIN32)
    typedef struct HINSTANCE__ *HINSTANCE;
    #if defined(_WIN64)
        typedef int64_t(__stdcall *FARPROC)();
    #else
        typedef int(__stdcall *FARPROC)();
    #endif
    extern "C" __declspec(dllimport) HINSTANCE __stdcall LoadLibraryA(char const *lpLibFileName);
    extern "C" __declspec(dllimport) int __stdcall FreeLibrary(HINSTANCE hLibModule);
    extern "C" __declspec(dllimport) FARPROC __stdcall GetProcAddress(HINSTANCE hModule, const char *lpProcName);
#endif

class flextDynamicLoader {
public:
    flextDynamicLoader() : m_success(false), m_library(nullptr) {
    }
    ~flextDynamicLoader() {
        if (m_library) {
#if defined(__linux__) || defined(__APPLE__)
            dlclose(m_library);
#elif defined(_WIN32)
            ::FreeLibrary(m_library);
#else
#error unsupported platform
#endif
        }
    }

    bool init() {
        if (m_success) return true;
#if defined(__linux__)
        m_library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
        if (m_library == nullptr) {
            m_library = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
        }
        m_success = (m_library != nullptr);
#elif defined(__APPLE__)
        m_library = dlopen("libvulkan.dylib", RTLD_NOW | RTLD_LOCAL);
        m_success = (m_library != nullptr);
#elif defined(_WIN32)
        m_library = ::LoadLibraryA("vulkan-1.dll");
        m_success = (m_library != nullptr);
#else
#error unsupported platform
#endif
        return m_success;
    }

#if defined(__linux__) || defined(__APPLE__)
    void*
#elif defined(_WIN32)
    FARPROC
#else
#error unsupported platform
#endif
    getProcAddress(const char *function) const {
#if defined(__linux__) || defined(__APPLE__)
        return dlsym(m_library, function);
#elif defined(_WIN32)
        return ::GetProcAddress(m_library, function);
#else
#error unsupported platform
#endif
    }

private:
    bool m_success;
#if defined(__linux__) || defined(__APPLE__)
    void *m_library;
#elif defined(_WIN32)
    ::HINSTANCE m_library;
#else
#error unsupported platform
#endif
};

void(VKAPI_PTR *flextvkDestroySurfaceKHR)(VkInstance, VkSurfaceKHR, const VkAllocationCallbacks*) = nullptr;
VkResult(VKAPI_PTR *flextvkGetPhysicalDeviceSurfaceCapabilitiesKHR)(VkPhysicalDevice, VkSurfaceKHR, VkSurfaceCapabilitiesKHR*) = nullptr;
VkResult(VKAPI_PTR *flextvkGetPhysicalDeviceSurfaceFormatsKHR)(VkPhysicalDevice, VkSurfaceKHR, uint32_t*, VkSurfaceFormatKHR*) = nullptr;
VkResult(VKAPI_PTR *flextvkGetPhysicalDeviceSurfacePresentModesKHR)(VkPhysicalDevice, VkSurfaceKHR, uint32_t*, VkPresentModeKHR*) = nullptr;
VkResult(VKAPI_PTR *flextvkGetPhysicalDeviceSurfaceSupportKHR)(VkPhysicalDevice, uint32_t, VkSurfaceKHR, VkBool32*) = nullptr;
VkResult(VKAPI_PTR *flextvkAcquireNextImage2KHR)(VkDevice, const VkAcquireNextImageInfoKHR*, uint32_t*) = nullptr;
VkResult(VKAPI_PTR *flextvkAcquireNextImageKHR)(VkDevice, VkSwapchainKHR, uint64_t, VkSemaphore, VkFence, uint32_t*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateSwapchainKHR)(VkDevice, const VkSwapchainCreateInfoKHR*, const VkAllocationCallbacks*, VkSwapchainKHR*) = nullptr;
void(VKAPI_PTR *flextvkDestroySwapchainKHR)(VkDevice, VkSwapchainKHR, const VkAllocationCallbacks*) = nullptr;
VkResult(VKAPI_PTR *flextvkGetDeviceGroupPresentCapabilitiesKHR)(VkDevice, VkDeviceGroupPresentCapabilitiesKHR*) = nullptr;
VkResult(VKAPI_PTR *flextvkGetDeviceGroupSurfacePresentModesKHR)(VkDevice, VkSurfaceKHR, VkDeviceGroupPresentModeFlagsKHR*) = nullptr;
VkResult(VKAPI_PTR *flextvkGetPhysicalDevicePresentRectanglesKHR)(VkPhysicalDevice, VkSurfaceKHR, uint32_t*, VkRect2D*) = nullptr;
VkResult(VKAPI_PTR *flextvkGetSwapchainImagesKHR)(VkDevice, VkSwapchainKHR, uint32_t*, VkImage*) = nullptr;
VkResult(VKAPI_PTR *flextvkQueuePresentKHR)(VkQueue, const VkPresentInfoKHR*) = nullptr;
VkResult(VKAPI_PTR *flextvkAllocateCommandBuffers)(VkDevice, const VkCommandBufferAllocateInfo*, VkCommandBuffer*) = nullptr;
VkResult(VKAPI_PTR *flextvkAllocateDescriptorSets)(VkDevice, const VkDescriptorSetAllocateInfo*, VkDescriptorSet*) = nullptr;
VkResult(VKAPI_PTR *flextvkAllocateMemory)(VkDevice, const VkMemoryAllocateInfo*, const VkAllocationCallbacks*, VkDeviceMemory*) = nullptr;
VkResult(VKAPI_PTR *flextvkBeginCommandBuffer)(VkCommandBuffer, const VkCommandBufferBeginInfo*) = nullptr;
VkResult(VKAPI_PTR *flextvkBindBufferMemory)(VkDevice, VkBuffer, VkDeviceMemory, VkDeviceSize) = nullptr;
VkResult(VKAPI_PTR *flextvkBindImageMemory)(VkDevice, VkImage, VkDeviceMemory, VkDeviceSize) = nullptr;
void(VKAPI_PTR *flextvkCmdBeginQuery)(VkCommandBuffer, VkQueryPool, uint32_t, VkQueryControlFlags) = nullptr;
void(VKAPI_PTR *flextvkCmdBeginRenderPass)(VkCommandBuffer, const VkRenderPassBeginInfo*, VkSubpassContents) = nullptr;
void(VKAPI_PTR *flextvkCmdBindDescriptorSets)(VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t, uint32_t, const VkDescriptorSet*, uint32_t, const uint32_t*) = nullptr;
void(VKAPI_PTR *flextvkCmdBindIndexBuffer)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkIndexType) = nullptr;
void(VKAPI_PTR *flextvkCmdBindPipeline)(VkCommandBuffer, VkPipelineBindPoint, VkPipeline) = nullptr;
void(VKAPI_PTR *flextvkCmdBindVertexBuffers)(VkCommandBuffer, uint32_t, uint32_t, const VkBuffer*, const VkDeviceSize*) = nullptr;
void(VKAPI_PTR *flextvkCmdBlitImage)(VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageBlit*, VkFilter) = nullptr;
void(VKAPI_PTR *flextvkCmdClearAttachments)(VkCommandBuffer, uint32_t, const VkClearAttachment*, uint32_t, const VkClearRect*) = nullptr;
void(VKAPI_PTR *flextvkCmdClearColorImage)(VkCommandBuffer, VkImage, VkImageLayout, const VkClearColorValue*, uint32_t, const VkImageSubresourceRange*) = nullptr;
void(VKAPI_PTR *flextvkCmdClearDepthStencilImage)(VkCommandBuffer, VkImage, VkImageLayout, const VkClearDepthStencilValue*, uint32_t, const VkImageSubresourceRange*) = nullptr;
void(VKAPI_PTR *flextvkCmdCopyBuffer)(VkCommandBuffer, VkBuffer, VkBuffer, uint32_t, const VkBufferCopy*) = nullptr;
void(VKAPI_PTR *flextvkCmdCopyBufferToImage)(VkCommandBuffer, VkBuffer, VkImage, VkImageLayout, uint32_t, const VkBufferImageCopy*) = nullptr;
void(VKAPI_PTR *flextvkCmdCopyImage)(VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageCopy*) = nullptr;
void(VKAPI_PTR *flextvkCmdCopyImageToBuffer)(VkCommandBuffer, VkImage, VkImageLayout, VkBuffer, uint32_t, const VkBufferImageCopy*) = nullptr;
void(VKAPI_PTR *flextvkCmdCopyQueryPoolResults)(VkCommandBuffer, VkQueryPool, uint32_t, uint32_t, VkBuffer, VkDeviceSize, VkDeviceSize, VkQueryResultFlags) = nullptr;
void(VKAPI_PTR *flextvkCmdDispatch)(VkCommandBuffer, uint32_t, uint32_t, uint32_t) = nullptr;
void(VKAPI_PTR *flextvkCmdDispatchIndirect)(VkCommandBuffer, VkBuffer, VkDeviceSize) = nullptr;
void(VKAPI_PTR *flextvkCmdDraw)(VkCommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t) = nullptr;
void(VKAPI_PTR *flextvkCmdDrawIndexed)(VkCommandBuffer, uint32_t, uint32_t, uint32_t, int32_t, uint32_t) = nullptr;
void(VKAPI_PTR *flextvkCmdDrawIndexedIndirect)(VkCommandBuffer, VkBuffer, VkDeviceSize, uint32_t, uint32_t) = nullptr;
void(VKAPI_PTR *flextvkCmdDrawIndirect)(VkCommandBuffer, VkBuffer, VkDeviceSize, uint32_t, uint32_t) = nullptr;
void(VKAPI_PTR *flextvkCmdEndQuery)(VkCommandBuffer, VkQueryPool, uint32_t) = nullptr;
void(VKAPI_PTR *flextvkCmdEndRenderPass)(VkCommandBuffer) = nullptr;
void(VKAPI_PTR *flextvkCmdExecuteCommands)(VkCommandBuffer, uint32_t, const VkCommandBuffer*) = nullptr;
void(VKAPI_PTR *flextvkCmdFillBuffer)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkDeviceSize, uint32_t) = nullptr;
void(VKAPI_PTR *flextvkCmdNextSubpass)(VkCommandBuffer, VkSubpassContents) = nullptr;
void(VKAPI_PTR *flextvkCmdPipelineBarrier)(VkCommandBuffer, VkPipelineStageFlags, VkPipelineStageFlags, VkDependencyFlags, uint32_t, const VkMemoryBarrier*, uint32_t, const VkBufferMemoryBarrier*, uint32_t, const VkImageMemoryBarrier*) = nullptr;
void(VKAPI_PTR *flextvkCmdPushConstants)(VkCommandBuffer, VkPipelineLayout, VkShaderStageFlags, uint32_t, uint32_t, const void*) = nullptr;
void(VKAPI_PTR *flextvkCmdResetEvent)(VkCommandBuffer, VkEvent, VkPipelineStageFlags) = nullptr;
void(VKAPI_PTR *flextvkCmdResetQueryPool)(VkCommandBuffer, VkQueryPool, uint32_t, uint32_t) = nullptr;
void(VKAPI_PTR *flextvkCmdResolveImage)(VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageResolve*) = nullptr;
void(VKAPI_PTR *flextvkCmdSetBlendConstants)(VkCommandBuffer, const float [4]) = nullptr;
void(VKAPI_PTR *flextvkCmdSetDepthBias)(VkCommandBuffer, float, float, float) = nullptr;
void(VKAPI_PTR *flextvkCmdSetDepthBounds)(VkCommandBuffer, float, float) = nullptr;
void(VKAPI_PTR *flextvkCmdSetEvent)(VkCommandBuffer, VkEvent, VkPipelineStageFlags) = nullptr;
void(VKAPI_PTR *flextvkCmdSetLineWidth)(VkCommandBuffer, float) = nullptr;
void(VKAPI_PTR *flextvkCmdSetScissor)(VkCommandBuffer, uint32_t, uint32_t, const VkRect2D*) = nullptr;
void(VKAPI_PTR *flextvkCmdSetStencilCompareMask)(VkCommandBuffer, VkStencilFaceFlags, uint32_t) = nullptr;
void(VKAPI_PTR *flextvkCmdSetStencilReference)(VkCommandBuffer, VkStencilFaceFlags, uint32_t) = nullptr;
void(VKAPI_PTR *flextvkCmdSetStencilWriteMask)(VkCommandBuffer, VkStencilFaceFlags, uint32_t) = nullptr;
void(VKAPI_PTR *flextvkCmdSetViewport)(VkCommandBuffer, uint32_t, uint32_t, const VkViewport*) = nullptr;
void(VKAPI_PTR *flextvkCmdUpdateBuffer)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkDeviceSize, const void*) = nullptr;
void(VKAPI_PTR *flextvkCmdWaitEvents)(VkCommandBuffer, uint32_t, const VkEvent*, VkPipelineStageFlags, VkPipelineStageFlags, uint32_t, const VkMemoryBarrier*, uint32_t, const VkBufferMemoryBarrier*, uint32_t, const VkImageMemoryBarrier*) = nullptr;
void(VKAPI_PTR *flextvkCmdWriteTimestamp)(VkCommandBuffer, VkPipelineStageFlagBits, VkQueryPool, uint32_t) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateBuffer)(VkDevice, const VkBufferCreateInfo*, const VkAllocationCallbacks*, VkBuffer*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateBufferView)(VkDevice, const VkBufferViewCreateInfo*, const VkAllocationCallbacks*, VkBufferView*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateCommandPool)(VkDevice, const VkCommandPoolCreateInfo*, const VkAllocationCallbacks*, VkCommandPool*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateComputePipelines)(VkDevice, VkPipelineCache, uint32_t, const VkComputePipelineCreateInfo*, const VkAllocationCallbacks*, VkPipeline*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateDescriptorPool)(VkDevice, const VkDescriptorPoolCreateInfo*, const VkAllocationCallbacks*, VkDescriptorPool*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateDescriptorSetLayout)(VkDevice, const VkDescriptorSetLayoutCreateInfo*, const VkAllocationCallbacks*, VkDescriptorSetLayout*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateDevice)(VkPhysicalDevice, const VkDeviceCreateInfo*, const VkAllocationCallbacks*, VkDevice*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateEvent)(VkDevice, const VkEventCreateInfo*, const VkAllocationCallbacks*, VkEvent*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateFence)(VkDevice, const VkFenceCreateInfo*, const VkAllocationCallbacks*, VkFence*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateFramebuffer)(VkDevice, const VkFramebufferCreateInfo*, const VkAllocationCallbacks*, VkFramebuffer*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateGraphicsPipelines)(VkDevice, VkPipelineCache, uint32_t, const VkGraphicsPipelineCreateInfo*, const VkAllocationCallbacks*, VkPipeline*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateImage)(VkDevice, const VkImageCreateInfo*, const VkAllocationCallbacks*, VkImage*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateImageView)(VkDevice, const VkImageViewCreateInfo*, const VkAllocationCallbacks*, VkImageView*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateInstance)(const VkInstanceCreateInfo*, const VkAllocationCallbacks*, VkInstance*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreatePipelineCache)(VkDevice, const VkPipelineCacheCreateInfo*, const VkAllocationCallbacks*, VkPipelineCache*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreatePipelineLayout)(VkDevice, const VkPipelineLayoutCreateInfo*, const VkAllocationCallbacks*, VkPipelineLayout*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateQueryPool)(VkDevice, const VkQueryPoolCreateInfo*, const VkAllocationCallbacks*, VkQueryPool*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateRenderPass)(VkDevice, const VkRenderPassCreateInfo*, const VkAllocationCallbacks*, VkRenderPass*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateSampler)(VkDevice, const VkSamplerCreateInfo*, const VkAllocationCallbacks*, VkSampler*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateSemaphore)(VkDevice, const VkSemaphoreCreateInfo*, const VkAllocationCallbacks*, VkSemaphore*) = nullptr;
VkResult(VKAPI_PTR *flextvkCreateShaderModule)(VkDevice, const VkShaderModuleCreateInfo*, const VkAllocationCallbacks*, VkShaderModule*) = nullptr;
void(VKAPI_PTR *flextvkDestroyBuffer)(VkDevice, VkBuffer, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroyBufferView)(VkDevice, VkBufferView, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroyCommandPool)(VkDevice, VkCommandPool, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroyDescriptorPool)(VkDevice, VkDescriptorPool, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroyDescriptorSetLayout)(VkDevice, VkDescriptorSetLayout, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroyDevice)(VkDevice, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroyEvent)(VkDevice, VkEvent, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroyFence)(VkDevice, VkFence, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroyFramebuffer)(VkDevice, VkFramebuffer, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroyImage)(VkDevice, VkImage, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroyImageView)(VkDevice, VkImageView, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroyInstance)(VkInstance, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroyPipeline)(VkDevice, VkPipeline, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroyPipelineCache)(VkDevice, VkPipelineCache, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroyPipelineLayout)(VkDevice, VkPipelineLayout, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroyQueryPool)(VkDevice, VkQueryPool, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroyRenderPass)(VkDevice, VkRenderPass, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroySampler)(VkDevice, VkSampler, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroySemaphore)(VkDevice, VkSemaphore, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkDestroyShaderModule)(VkDevice, VkShaderModule, const VkAllocationCallbacks*) = nullptr;
VkResult(VKAPI_PTR *flextvkDeviceWaitIdle)(VkDevice) = nullptr;
VkResult(VKAPI_PTR *flextvkEndCommandBuffer)(VkCommandBuffer) = nullptr;
VkResult(VKAPI_PTR *flextvkEnumerateDeviceExtensionProperties)(VkPhysicalDevice, const char*, uint32_t*, VkExtensionProperties*) = nullptr;
VkResult(VKAPI_PTR *flextvkEnumerateDeviceLayerProperties)(VkPhysicalDevice, uint32_t*, VkLayerProperties*) = nullptr;
VkResult(VKAPI_PTR *flextvkEnumerateInstanceExtensionProperties)(const char*, uint32_t*, VkExtensionProperties*) = nullptr;
VkResult(VKAPI_PTR *flextvkEnumerateInstanceLayerProperties)(uint32_t*, VkLayerProperties*) = nullptr;
VkResult(VKAPI_PTR *flextvkEnumeratePhysicalDevices)(VkInstance, uint32_t*, VkPhysicalDevice*) = nullptr;
VkResult(VKAPI_PTR *flextvkFlushMappedMemoryRanges)(VkDevice, uint32_t, const VkMappedMemoryRange*) = nullptr;
void(VKAPI_PTR *flextvkFreeCommandBuffers)(VkDevice, VkCommandPool, uint32_t, const VkCommandBuffer*) = nullptr;
VkResult(VKAPI_PTR *flextvkFreeDescriptorSets)(VkDevice, VkDescriptorPool, uint32_t, const VkDescriptorSet*) = nullptr;
void(VKAPI_PTR *flextvkFreeMemory)(VkDevice, VkDeviceMemory, const VkAllocationCallbacks*) = nullptr;
void(VKAPI_PTR *flextvkGetBufferMemoryRequirements)(VkDevice, VkBuffer, VkMemoryRequirements*) = nullptr;
void(VKAPI_PTR *flextvkGetDeviceMemoryCommitment)(VkDevice, VkDeviceMemory, VkDeviceSize*) = nullptr;
PFN_vkVoidFunction(VKAPI_PTR *flextvkGetDeviceProcAddr)(VkDevice, const char*) = nullptr;
void(VKAPI_PTR *flextvkGetDeviceQueue)(VkDevice, uint32_t, uint32_t, VkQueue*) = nullptr;
VkResult(VKAPI_PTR *flextvkGetEventStatus)(VkDevice, VkEvent) = nullptr;
VkResult(VKAPI_PTR *flextvkGetFenceStatus)(VkDevice, VkFence) = nullptr;
void(VKAPI_PTR *flextvkGetImageMemoryRequirements)(VkDevice, VkImage, VkMemoryRequirements*) = nullptr;
void(VKAPI_PTR *flextvkGetImageSparseMemoryRequirements)(VkDevice, VkImage, uint32_t*, VkSparseImageMemoryRequirements*) = nullptr;
void(VKAPI_PTR *flextvkGetImageSubresourceLayout)(VkDevice, VkImage, const VkImageSubresource*, VkSubresourceLayout*) = nullptr;
PFN_vkVoidFunction(VKAPI_PTR *flextvkGetInstanceProcAddr)(VkInstance, const char*) = nullptr;
void(VKAPI_PTR *flextvkGetPhysicalDeviceFeatures)(VkPhysicalDevice, VkPhysicalDeviceFeatures*) = nullptr;
void(VKAPI_PTR *flextvkGetPhysicalDeviceFormatProperties)(VkPhysicalDevice, VkFormat, VkFormatProperties*) = nullptr;
VkResult(VKAPI_PTR *flextvkGetPhysicalDeviceImageFormatProperties)(VkPhysicalDevice, VkFormat, VkImageType, VkImageTiling, VkImageUsageFlags, VkImageCreateFlags, VkImageFormatProperties*) = nullptr;
void(VKAPI_PTR *flextvkGetPhysicalDeviceMemoryProperties)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties*) = nullptr;
void(VKAPI_PTR *flextvkGetPhysicalDeviceProperties)(VkPhysicalDevice, VkPhysicalDeviceProperties*) = nullptr;
void(VKAPI_PTR *flextvkGetPhysicalDeviceQueueFamilyProperties)(VkPhysicalDevice, uint32_t*, VkQueueFamilyProperties*) = nullptr;
void(VKAPI_PTR *flextvkGetPhysicalDeviceSparseImageFormatProperties)(VkPhysicalDevice, VkFormat, VkImageType, VkSampleCountFlagBits, VkImageUsageFlags, VkImageTiling, uint32_t*, VkSparseImageFormatProperties*) = nullptr;
VkResult(VKAPI_PTR *flextvkGetPipelineCacheData)(VkDevice, VkPipelineCache, size_t*, void*) = nullptr;
VkResult(VKAPI_PTR *flextvkGetQueryPoolResults)(VkDevice, VkQueryPool, uint32_t, uint32_t, size_t, void*, VkDeviceSize, VkQueryResultFlags) = nullptr;
void(VKAPI_PTR *flextvkGetRenderAreaGranularity)(VkDevice, VkRenderPass, VkExtent2D*) = nullptr;
VkResult(VKAPI_PTR *flextvkInvalidateMappedMemoryRanges)(VkDevice, uint32_t, const VkMappedMemoryRange*) = nullptr;
VkResult(VKAPI_PTR *flextvkMapMemory)(VkDevice, VkDeviceMemory, VkDeviceSize, VkDeviceSize, VkMemoryMapFlags, void**) = nullptr;
VkResult(VKAPI_PTR *flextvkMergePipelineCaches)(VkDevice, VkPipelineCache, uint32_t, const VkPipelineCache*) = nullptr;
VkResult(VKAPI_PTR *flextvkQueueBindSparse)(VkQueue, uint32_t, const VkBindSparseInfo*, VkFence) = nullptr;
VkResult(VKAPI_PTR *flextvkQueueSubmit)(VkQueue, uint32_t, const VkSubmitInfo*, VkFence) = nullptr;
VkResult(VKAPI_PTR *flextvkQueueWaitIdle)(VkQueue) = nullptr;
VkResult(VKAPI_PTR *flextvkResetCommandBuffer)(VkCommandBuffer, VkCommandBufferResetFlags) = nullptr;
VkResult(VKAPI_PTR *flextvkResetCommandPool)(VkDevice, VkCommandPool, VkCommandPoolResetFlags) = nullptr;
VkResult(VKAPI_PTR *flextvkResetDescriptorPool)(VkDevice, VkDescriptorPool, VkDescriptorPoolResetFlags) = nullptr;
VkResult(VKAPI_PTR *flextvkResetEvent)(VkDevice, VkEvent) = nullptr;
VkResult(VKAPI_PTR *flextvkResetFences)(VkDevice, uint32_t, const VkFence*) = nullptr;
VkResult(VKAPI_PTR *flextvkSetEvent)(VkDevice, VkEvent) = nullptr;
void(VKAPI_PTR *flextvkUnmapMemory)(VkDevice, VkDeviceMemory) = nullptr;
void(VKAPI_PTR *flextvkUpdateDescriptorSets)(VkDevice, uint32_t, const VkWriteDescriptorSet*, uint32_t, const VkCopyDescriptorSet*) = nullptr;
VkResult(VKAPI_PTR *flextvkWaitForFences)(VkDevice, uint32_t, const VkFence*, VkBool32, uint64_t) = nullptr;

bool flextVkInit() {
    static flextDynamicLoader loader;
    if (!loader.init()) return false;
    flextvkCreateInstance = reinterpret_cast<VkResult(VKAPI_PTR*)(const VkInstanceCreateInfo*, const VkAllocationCallbacks*, VkInstance*)>(loader.getProcAddress("vkCreateInstance"));
    flextvkEnumerateInstanceExtensionProperties = reinterpret_cast<VkResult(VKAPI_PTR*)(const char*, uint32_t*, VkExtensionProperties*)>(loader.getProcAddress("vkEnumerateInstanceExtensionProperties"));
    flextvkEnumerateInstanceLayerProperties = reinterpret_cast<VkResult(VKAPI_PTR*)(uint32_t*, VkLayerProperties*)>(loader.getProcAddress("vkEnumerateInstanceLayerProperties"));
    flextvkGetInstanceProcAddr = reinterpret_cast<PFN_vkVoidFunction(VKAPI_PTR*)(VkInstance, const char*)>(loader.getProcAddress("vkGetInstanceProcAddr"));
    return true;
}

void flextVkInitInstance(VkInstance instance) {
    flextvkDestroySurfaceKHR = reinterpret_cast<void(VKAPI_PTR*)(VkInstance, VkSurfaceKHR, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroySurfaceKHR"));
    flextvkGetPhysicalDeviceSurfaceCapabilitiesKHR = reinterpret_cast<VkResult(VKAPI_PTR*)(VkPhysicalDevice, VkSurfaceKHR, VkSurfaceCapabilitiesKHR*)>(flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    flextvkGetPhysicalDeviceSurfaceFormatsKHR = reinterpret_cast<VkResult(VKAPI_PTR*)(VkPhysicalDevice, VkSurfaceKHR, uint32_t*, VkSurfaceFormatKHR*)>(flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
    flextvkGetPhysicalDeviceSurfacePresentModesKHR = reinterpret_cast<VkResult(VKAPI_PTR*)(VkPhysicalDevice, VkSurfaceKHR, uint32_t*, VkPresentModeKHR*)>(flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));
    flextvkGetPhysicalDeviceSurfaceSupportKHR = reinterpret_cast<VkResult(VKAPI_PTR*)(VkPhysicalDevice, uint32_t, VkSurfaceKHR, VkBool32*)>(flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
    flextvkAcquireNextImage2KHR = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkAcquireNextImageInfoKHR*, uint32_t*)>(flextvkGetInstanceProcAddr(instance, "vkAcquireNextImage2KHR"));
    flextvkAcquireNextImageKHR = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkSwapchainKHR, uint64_t, VkSemaphore, VkFence, uint32_t*)>(flextvkGetInstanceProcAddr(instance, "vkAcquireNextImageKHR"));
    flextvkCreateSwapchainKHR = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkSwapchainCreateInfoKHR*, const VkAllocationCallbacks*, VkSwapchainKHR*)>(flextvkGetInstanceProcAddr(instance, "vkCreateSwapchainKHR"));
    flextvkDestroySwapchainKHR = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkSwapchainKHR, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroySwapchainKHR"));
    flextvkGetDeviceGroupPresentCapabilitiesKHR = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkDeviceGroupPresentCapabilitiesKHR*)>(flextvkGetInstanceProcAddr(instance, "vkGetDeviceGroupPresentCapabilitiesKHR"));
    flextvkGetDeviceGroupSurfacePresentModesKHR = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkSurfaceKHR, VkDeviceGroupPresentModeFlagsKHR*)>(flextvkGetInstanceProcAddr(instance, "vkGetDeviceGroupSurfacePresentModesKHR"));
    flextvkGetPhysicalDevicePresentRectanglesKHR = reinterpret_cast<VkResult(VKAPI_PTR*)(VkPhysicalDevice, VkSurfaceKHR, uint32_t*, VkRect2D*)>(flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDevicePresentRectanglesKHR"));
    flextvkGetSwapchainImagesKHR = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkSwapchainKHR, uint32_t*, VkImage*)>(flextvkGetInstanceProcAddr(instance, "vkGetSwapchainImagesKHR"));
    flextvkQueuePresentKHR = reinterpret_cast<VkResult(VKAPI_PTR*)(VkQueue, const VkPresentInfoKHR*)>(flextvkGetInstanceProcAddr(instance, "vkQueuePresentKHR"));
    flextvkAllocateCommandBuffers = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkCommandBufferAllocateInfo*, VkCommandBuffer*)>(flextvkGetInstanceProcAddr(instance, "vkAllocateCommandBuffers"));
    flextvkAllocateDescriptorSets = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkDescriptorSetAllocateInfo*, VkDescriptorSet*)>(flextvkGetInstanceProcAddr(instance, "vkAllocateDescriptorSets"));
    flextvkAllocateMemory = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkMemoryAllocateInfo*, const VkAllocationCallbacks*, VkDeviceMemory*)>(flextvkGetInstanceProcAddr(instance, "vkAllocateMemory"));
    flextvkBeginCommandBuffer = reinterpret_cast<VkResult(VKAPI_PTR*)(VkCommandBuffer, const VkCommandBufferBeginInfo*)>(flextvkGetInstanceProcAddr(instance, "vkBeginCommandBuffer"));
    flextvkBindBufferMemory = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkBuffer, VkDeviceMemory, VkDeviceSize)>(flextvkGetInstanceProcAddr(instance, "vkBindBufferMemory"));
    flextvkBindImageMemory = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkImage, VkDeviceMemory, VkDeviceSize)>(flextvkGetInstanceProcAddr(instance, "vkBindImageMemory"));
    flextvkCmdBeginQuery = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkQueryPool, uint32_t, VkQueryControlFlags)>(flextvkGetInstanceProcAddr(instance, "vkCmdBeginQuery"));
    flextvkCmdBeginRenderPass = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, const VkRenderPassBeginInfo*, VkSubpassContents)>(flextvkGetInstanceProcAddr(instance, "vkCmdBeginRenderPass"));
    flextvkCmdBindDescriptorSets = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t, uint32_t, const VkDescriptorSet*, uint32_t, const uint32_t*)>(flextvkGetInstanceProcAddr(instance, "vkCmdBindDescriptorSets"));
    flextvkCmdBindIndexBuffer = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkIndexType)>(flextvkGetInstanceProcAddr(instance, "vkCmdBindIndexBuffer"));
    flextvkCmdBindPipeline = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkPipelineBindPoint, VkPipeline)>(flextvkGetInstanceProcAddr(instance, "vkCmdBindPipeline"));
    flextvkCmdBindVertexBuffers = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, uint32_t, uint32_t, const VkBuffer*, const VkDeviceSize*)>(flextvkGetInstanceProcAddr(instance, "vkCmdBindVertexBuffers"));
    flextvkCmdBlitImage = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageBlit*, VkFilter)>(flextvkGetInstanceProcAddr(instance, "vkCmdBlitImage"));
    flextvkCmdClearAttachments = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, uint32_t, const VkClearAttachment*, uint32_t, const VkClearRect*)>(flextvkGetInstanceProcAddr(instance, "vkCmdClearAttachments"));
    flextvkCmdClearColorImage = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkImage, VkImageLayout, const VkClearColorValue*, uint32_t, const VkImageSubresourceRange*)>(flextvkGetInstanceProcAddr(instance, "vkCmdClearColorImage"));
    flextvkCmdClearDepthStencilImage = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkImage, VkImageLayout, const VkClearDepthStencilValue*, uint32_t, const VkImageSubresourceRange*)>(flextvkGetInstanceProcAddr(instance, "vkCmdClearDepthStencilImage"));
    flextvkCmdCopyBuffer = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkBuffer, VkBuffer, uint32_t, const VkBufferCopy*)>(flextvkGetInstanceProcAddr(instance, "vkCmdCopyBuffer"));
    flextvkCmdCopyBufferToImage = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkBuffer, VkImage, VkImageLayout, uint32_t, const VkBufferImageCopy*)>(flextvkGetInstanceProcAddr(instance, "vkCmdCopyBufferToImage"));
    flextvkCmdCopyImage = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageCopy*)>(flextvkGetInstanceProcAddr(instance, "vkCmdCopyImage"));
    flextvkCmdCopyImageToBuffer = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkImage, VkImageLayout, VkBuffer, uint32_t, const VkBufferImageCopy*)>(flextvkGetInstanceProcAddr(instance, "vkCmdCopyImageToBuffer"));
    flextvkCmdCopyQueryPoolResults = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkQueryPool, uint32_t, uint32_t, VkBuffer, VkDeviceSize, VkDeviceSize, VkQueryResultFlags)>(flextvkGetInstanceProcAddr(instance, "vkCmdCopyQueryPoolResults"));
    flextvkCmdDispatch = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, uint32_t, uint32_t, uint32_t)>(flextvkGetInstanceProcAddr(instance, "vkCmdDispatch"));
    flextvkCmdDispatchIndirect = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkBuffer, VkDeviceSize)>(flextvkGetInstanceProcAddr(instance, "vkCmdDispatchIndirect"));
    flextvkCmdDraw = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t)>(flextvkGetInstanceProcAddr(instance, "vkCmdDraw"));
    flextvkCmdDrawIndexed = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, uint32_t, uint32_t, uint32_t, int32_t, uint32_t)>(flextvkGetInstanceProcAddr(instance, "vkCmdDrawIndexed"));
    flextvkCmdDrawIndexedIndirect = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkBuffer, VkDeviceSize, uint32_t, uint32_t)>(flextvkGetInstanceProcAddr(instance, "vkCmdDrawIndexedIndirect"));
    flextvkCmdDrawIndirect = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkBuffer, VkDeviceSize, uint32_t, uint32_t)>(flextvkGetInstanceProcAddr(instance, "vkCmdDrawIndirect"));
    flextvkCmdEndQuery = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkQueryPool, uint32_t)>(flextvkGetInstanceProcAddr(instance, "vkCmdEndQuery"));
    flextvkCmdEndRenderPass = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer)>(flextvkGetInstanceProcAddr(instance, "vkCmdEndRenderPass"));
    flextvkCmdExecuteCommands = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, uint32_t, const VkCommandBuffer*)>(flextvkGetInstanceProcAddr(instance, "vkCmdExecuteCommands"));
    flextvkCmdFillBuffer = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkDeviceSize, uint32_t)>(flextvkGetInstanceProcAddr(instance, "vkCmdFillBuffer"));
    flextvkCmdNextSubpass = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkSubpassContents)>(flextvkGetInstanceProcAddr(instance, "vkCmdNextSubpass"));
    flextvkCmdPipelineBarrier = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkPipelineStageFlags, VkPipelineStageFlags, VkDependencyFlags, uint32_t, const VkMemoryBarrier*, uint32_t, const VkBufferMemoryBarrier*, uint32_t, const VkImageMemoryBarrier*)>(flextvkGetInstanceProcAddr(instance, "vkCmdPipelineBarrier"));
    flextvkCmdPushConstants = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkPipelineLayout, VkShaderStageFlags, uint32_t, uint32_t, const void*)>(flextvkGetInstanceProcAddr(instance, "vkCmdPushConstants"));
    flextvkCmdResetEvent = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkEvent, VkPipelineStageFlags)>(flextvkGetInstanceProcAddr(instance, "vkCmdResetEvent"));
    flextvkCmdResetQueryPool = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkQueryPool, uint32_t, uint32_t)>(flextvkGetInstanceProcAddr(instance, "vkCmdResetQueryPool"));
    flextvkCmdResolveImage = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageResolve*)>(flextvkGetInstanceProcAddr(instance, "vkCmdResolveImage"));
    flextvkCmdSetBlendConstants = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, const float [4])>(flextvkGetInstanceProcAddr(instance, "vkCmdSetBlendConstants"));
    flextvkCmdSetDepthBias = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, float, float, float)>(flextvkGetInstanceProcAddr(instance, "vkCmdSetDepthBias"));
    flextvkCmdSetDepthBounds = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, float, float)>(flextvkGetInstanceProcAddr(instance, "vkCmdSetDepthBounds"));
    flextvkCmdSetEvent = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkEvent, VkPipelineStageFlags)>(flextvkGetInstanceProcAddr(instance, "vkCmdSetEvent"));
    flextvkCmdSetLineWidth = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, float)>(flextvkGetInstanceProcAddr(instance, "vkCmdSetLineWidth"));
    flextvkCmdSetScissor = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, uint32_t, uint32_t, const VkRect2D*)>(flextvkGetInstanceProcAddr(instance, "vkCmdSetScissor"));
    flextvkCmdSetStencilCompareMask = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkStencilFaceFlags, uint32_t)>(flextvkGetInstanceProcAddr(instance, "vkCmdSetStencilCompareMask"));
    flextvkCmdSetStencilReference = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkStencilFaceFlags, uint32_t)>(flextvkGetInstanceProcAddr(instance, "vkCmdSetStencilReference"));
    flextvkCmdSetStencilWriteMask = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkStencilFaceFlags, uint32_t)>(flextvkGetInstanceProcAddr(instance, "vkCmdSetStencilWriteMask"));
    flextvkCmdSetViewport = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, uint32_t, uint32_t, const VkViewport*)>(flextvkGetInstanceProcAddr(instance, "vkCmdSetViewport"));
    flextvkCmdUpdateBuffer = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkDeviceSize, const void*)>(flextvkGetInstanceProcAddr(instance, "vkCmdUpdateBuffer"));
    flextvkCmdWaitEvents = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, uint32_t, const VkEvent*, VkPipelineStageFlags, VkPipelineStageFlags, uint32_t, const VkMemoryBarrier*, uint32_t, const VkBufferMemoryBarrier*, uint32_t, const VkImageMemoryBarrier*)>(flextvkGetInstanceProcAddr(instance, "vkCmdWaitEvents"));
    flextvkCmdWriteTimestamp = reinterpret_cast<void(VKAPI_PTR*)(VkCommandBuffer, VkPipelineStageFlagBits, VkQueryPool, uint32_t)>(flextvkGetInstanceProcAddr(instance, "vkCmdWriteTimestamp"));
    flextvkCreateBuffer = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkBufferCreateInfo*, const VkAllocationCallbacks*, VkBuffer*)>(flextvkGetInstanceProcAddr(instance, "vkCreateBuffer"));
    flextvkCreateBufferView = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkBufferViewCreateInfo*, const VkAllocationCallbacks*, VkBufferView*)>(flextvkGetInstanceProcAddr(instance, "vkCreateBufferView"));
    flextvkCreateCommandPool = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkCommandPoolCreateInfo*, const VkAllocationCallbacks*, VkCommandPool*)>(flextvkGetInstanceProcAddr(instance, "vkCreateCommandPool"));
    flextvkCreateComputePipelines = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkPipelineCache, uint32_t, const VkComputePipelineCreateInfo*, const VkAllocationCallbacks*, VkPipeline*)>(flextvkGetInstanceProcAddr(instance, "vkCreateComputePipelines"));
    flextvkCreateDescriptorPool = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkDescriptorPoolCreateInfo*, const VkAllocationCallbacks*, VkDescriptorPool*)>(flextvkGetInstanceProcAddr(instance, "vkCreateDescriptorPool"));
    flextvkCreateDescriptorSetLayout = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkDescriptorSetLayoutCreateInfo*, const VkAllocationCallbacks*, VkDescriptorSetLayout*)>(flextvkGetInstanceProcAddr(instance, "vkCreateDescriptorSetLayout"));
    flextvkCreateDevice = reinterpret_cast<VkResult(VKAPI_PTR*)(VkPhysicalDevice, const VkDeviceCreateInfo*, const VkAllocationCallbacks*, VkDevice*)>(flextvkGetInstanceProcAddr(instance, "vkCreateDevice"));
    flextvkCreateEvent = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkEventCreateInfo*, const VkAllocationCallbacks*, VkEvent*)>(flextvkGetInstanceProcAddr(instance, "vkCreateEvent"));
    flextvkCreateFence = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkFenceCreateInfo*, const VkAllocationCallbacks*, VkFence*)>(flextvkGetInstanceProcAddr(instance, "vkCreateFence"));
    flextvkCreateFramebuffer = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkFramebufferCreateInfo*, const VkAllocationCallbacks*, VkFramebuffer*)>(flextvkGetInstanceProcAddr(instance, "vkCreateFramebuffer"));
    flextvkCreateGraphicsPipelines = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkPipelineCache, uint32_t, const VkGraphicsPipelineCreateInfo*, const VkAllocationCallbacks*, VkPipeline*)>(flextvkGetInstanceProcAddr(instance, "vkCreateGraphicsPipelines"));
    flextvkCreateImage = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkImageCreateInfo*, const VkAllocationCallbacks*, VkImage*)>(flextvkGetInstanceProcAddr(instance, "vkCreateImage"));
    flextvkCreateImageView = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkImageViewCreateInfo*, const VkAllocationCallbacks*, VkImageView*)>(flextvkGetInstanceProcAddr(instance, "vkCreateImageView"));
    flextvkCreatePipelineCache = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkPipelineCacheCreateInfo*, const VkAllocationCallbacks*, VkPipelineCache*)>(flextvkGetInstanceProcAddr(instance, "vkCreatePipelineCache"));
    flextvkCreatePipelineLayout = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkPipelineLayoutCreateInfo*, const VkAllocationCallbacks*, VkPipelineLayout*)>(flextvkGetInstanceProcAddr(instance, "vkCreatePipelineLayout"));
    flextvkCreateQueryPool = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkQueryPoolCreateInfo*, const VkAllocationCallbacks*, VkQueryPool*)>(flextvkGetInstanceProcAddr(instance, "vkCreateQueryPool"));
    flextvkCreateRenderPass = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkRenderPassCreateInfo*, const VkAllocationCallbacks*, VkRenderPass*)>(flextvkGetInstanceProcAddr(instance, "vkCreateRenderPass"));
    flextvkCreateSampler = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkSamplerCreateInfo*, const VkAllocationCallbacks*, VkSampler*)>(flextvkGetInstanceProcAddr(instance, "vkCreateSampler"));
    flextvkCreateSemaphore = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkSemaphoreCreateInfo*, const VkAllocationCallbacks*, VkSemaphore*)>(flextvkGetInstanceProcAddr(instance, "vkCreateSemaphore"));
    flextvkCreateShaderModule = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, const VkShaderModuleCreateInfo*, const VkAllocationCallbacks*, VkShaderModule*)>(flextvkGetInstanceProcAddr(instance, "vkCreateShaderModule"));
    flextvkDestroyBuffer = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkBuffer, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroyBuffer"));
    flextvkDestroyBufferView = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkBufferView, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroyBufferView"));
    flextvkDestroyCommandPool = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkCommandPool, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroyCommandPool"));
    flextvkDestroyDescriptorPool = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkDescriptorPool, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroyDescriptorPool"));
    flextvkDestroyDescriptorSetLayout = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkDescriptorSetLayout, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroyDescriptorSetLayout"));
    flextvkDestroyDevice = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroyDevice"));
    flextvkDestroyEvent = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkEvent, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroyEvent"));
    flextvkDestroyFence = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkFence, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroyFence"));
    flextvkDestroyFramebuffer = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkFramebuffer, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroyFramebuffer"));
    flextvkDestroyImage = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkImage, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroyImage"));
    flextvkDestroyImageView = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkImageView, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroyImageView"));
    flextvkDestroyInstance = reinterpret_cast<void(VKAPI_PTR*)(VkInstance, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroyInstance"));
    flextvkDestroyPipeline = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkPipeline, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroyPipeline"));
    flextvkDestroyPipelineCache = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkPipelineCache, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroyPipelineCache"));
    flextvkDestroyPipelineLayout = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkPipelineLayout, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroyPipelineLayout"));
    flextvkDestroyQueryPool = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkQueryPool, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroyQueryPool"));
    flextvkDestroyRenderPass = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkRenderPass, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroyRenderPass"));
    flextvkDestroySampler = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkSampler, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroySampler"));
    flextvkDestroySemaphore = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkSemaphore, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroySemaphore"));
    flextvkDestroyShaderModule = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkShaderModule, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkDestroyShaderModule"));
    flextvkDeviceWaitIdle = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice)>(flextvkGetInstanceProcAddr(instance, "vkDeviceWaitIdle"));
    flextvkEndCommandBuffer = reinterpret_cast<VkResult(VKAPI_PTR*)(VkCommandBuffer)>(flextvkGetInstanceProcAddr(instance, "vkEndCommandBuffer"));
    flextvkEnumerateDeviceExtensionProperties = reinterpret_cast<VkResult(VKAPI_PTR*)(VkPhysicalDevice, const char*, uint32_t*, VkExtensionProperties*)>(flextvkGetInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties"));
    flextvkEnumerateDeviceLayerProperties = reinterpret_cast<VkResult(VKAPI_PTR*)(VkPhysicalDevice, uint32_t*, VkLayerProperties*)>(flextvkGetInstanceProcAddr(instance, "vkEnumerateDeviceLayerProperties"));
    flextvkEnumeratePhysicalDevices = reinterpret_cast<VkResult(VKAPI_PTR*)(VkInstance, uint32_t*, VkPhysicalDevice*)>(flextvkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices"));
    flextvkFlushMappedMemoryRanges = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, uint32_t, const VkMappedMemoryRange*)>(flextvkGetInstanceProcAddr(instance, "vkFlushMappedMemoryRanges"));
    flextvkFreeCommandBuffers = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkCommandPool, uint32_t, const VkCommandBuffer*)>(flextvkGetInstanceProcAddr(instance, "vkFreeCommandBuffers"));
    flextvkFreeDescriptorSets = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkDescriptorPool, uint32_t, const VkDescriptorSet*)>(flextvkGetInstanceProcAddr(instance, "vkFreeDescriptorSets"));
    flextvkFreeMemory = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkDeviceMemory, const VkAllocationCallbacks*)>(flextvkGetInstanceProcAddr(instance, "vkFreeMemory"));
    flextvkGetBufferMemoryRequirements = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkBuffer, VkMemoryRequirements*)>(flextvkGetInstanceProcAddr(instance, "vkGetBufferMemoryRequirements"));
    flextvkGetDeviceMemoryCommitment = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkDeviceMemory, VkDeviceSize*)>(flextvkGetInstanceProcAddr(instance, "vkGetDeviceMemoryCommitment"));
    flextvkGetDeviceProcAddr = reinterpret_cast<PFN_vkVoidFunction(VKAPI_PTR*)(VkDevice, const char*)>(flextvkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr"));
    flextvkGetDeviceQueue = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, uint32_t, uint32_t, VkQueue*)>(flextvkGetInstanceProcAddr(instance, "vkGetDeviceQueue"));
    flextvkGetEventStatus = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkEvent)>(flextvkGetInstanceProcAddr(instance, "vkGetEventStatus"));
    flextvkGetFenceStatus = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkFence)>(flextvkGetInstanceProcAddr(instance, "vkGetFenceStatus"));
    flextvkGetImageMemoryRequirements = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkImage, VkMemoryRequirements*)>(flextvkGetInstanceProcAddr(instance, "vkGetImageMemoryRequirements"));
    flextvkGetImageSparseMemoryRequirements = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkImage, uint32_t*, VkSparseImageMemoryRequirements*)>(flextvkGetInstanceProcAddr(instance, "vkGetImageSparseMemoryRequirements"));
    flextvkGetImageSubresourceLayout = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkImage, const VkImageSubresource*, VkSubresourceLayout*)>(flextvkGetInstanceProcAddr(instance, "vkGetImageSubresourceLayout"));
    flextvkGetPhysicalDeviceFeatures = reinterpret_cast<void(VKAPI_PTR*)(VkPhysicalDevice, VkPhysicalDeviceFeatures*)>(flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures"));
    flextvkGetPhysicalDeviceFormatProperties = reinterpret_cast<void(VKAPI_PTR*)(VkPhysicalDevice, VkFormat, VkFormatProperties*)>(flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFormatProperties"));
    flextvkGetPhysicalDeviceImageFormatProperties = reinterpret_cast<VkResult(VKAPI_PTR*)(VkPhysicalDevice, VkFormat, VkImageType, VkImageTiling, VkImageUsageFlags, VkImageCreateFlags, VkImageFormatProperties*)>(flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceImageFormatProperties"));
    flextvkGetPhysicalDeviceMemoryProperties = reinterpret_cast<void(VKAPI_PTR*)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties*)>(flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties"));
    flextvkGetPhysicalDeviceProperties = reinterpret_cast<void(VKAPI_PTR*)(VkPhysicalDevice, VkPhysicalDeviceProperties*)>(flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties"));
    flextvkGetPhysicalDeviceQueueFamilyProperties = reinterpret_cast<void(VKAPI_PTR*)(VkPhysicalDevice, uint32_t*, VkQueueFamilyProperties*)>(flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
    flextvkGetPhysicalDeviceSparseImageFormatProperties = reinterpret_cast<void(VKAPI_PTR*)(VkPhysicalDevice, VkFormat, VkImageType, VkSampleCountFlagBits, VkImageUsageFlags, VkImageTiling, uint32_t*, VkSparseImageFormatProperties*)>(flextvkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSparseImageFormatProperties"));
    flextvkGetPipelineCacheData = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkPipelineCache, size_t*, void*)>(flextvkGetInstanceProcAddr(instance, "vkGetPipelineCacheData"));
    flextvkGetQueryPoolResults = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkQueryPool, uint32_t, uint32_t, size_t, void*, VkDeviceSize, VkQueryResultFlags)>(flextvkGetInstanceProcAddr(instance, "vkGetQueryPoolResults"));
    flextvkGetRenderAreaGranularity = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkRenderPass, VkExtent2D*)>(flextvkGetInstanceProcAddr(instance, "vkGetRenderAreaGranularity"));
    flextvkInvalidateMappedMemoryRanges = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, uint32_t, const VkMappedMemoryRange*)>(flextvkGetInstanceProcAddr(instance, "vkInvalidateMappedMemoryRanges"));
    flextvkMapMemory = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkDeviceMemory, VkDeviceSize, VkDeviceSize, VkMemoryMapFlags, void**)>(flextvkGetInstanceProcAddr(instance, "vkMapMemory"));
    flextvkMergePipelineCaches = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkPipelineCache, uint32_t, const VkPipelineCache*)>(flextvkGetInstanceProcAddr(instance, "vkMergePipelineCaches"));
    flextvkQueueBindSparse = reinterpret_cast<VkResult(VKAPI_PTR*)(VkQueue, uint32_t, const VkBindSparseInfo*, VkFence)>(flextvkGetInstanceProcAddr(instance, "vkQueueBindSparse"));
    flextvkQueueSubmit = reinterpret_cast<VkResult(VKAPI_PTR*)(VkQueue, uint32_t, const VkSubmitInfo*, VkFence)>(flextvkGetInstanceProcAddr(instance, "vkQueueSubmit"));
    flextvkQueueWaitIdle = reinterpret_cast<VkResult(VKAPI_PTR*)(VkQueue)>(flextvkGetInstanceProcAddr(instance, "vkQueueWaitIdle"));
    flextvkResetCommandBuffer = reinterpret_cast<VkResult(VKAPI_PTR*)(VkCommandBuffer, VkCommandBufferResetFlags)>(flextvkGetInstanceProcAddr(instance, "vkResetCommandBuffer"));
    flextvkResetCommandPool = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkCommandPool, VkCommandPoolResetFlags)>(flextvkGetInstanceProcAddr(instance, "vkResetCommandPool"));
    flextvkResetDescriptorPool = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkDescriptorPool, VkDescriptorPoolResetFlags)>(flextvkGetInstanceProcAddr(instance, "vkResetDescriptorPool"));
    flextvkResetEvent = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkEvent)>(flextvkGetInstanceProcAddr(instance, "vkResetEvent"));
    flextvkResetFences = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, uint32_t, const VkFence*)>(flextvkGetInstanceProcAddr(instance, "vkResetFences"));
    flextvkSetEvent = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, VkEvent)>(flextvkGetInstanceProcAddr(instance, "vkSetEvent"));
    flextvkUnmapMemory = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, VkDeviceMemory)>(flextvkGetInstanceProcAddr(instance, "vkUnmapMemory"));
    flextvkUpdateDescriptorSets = reinterpret_cast<void(VKAPI_PTR*)(VkDevice, uint32_t, const VkWriteDescriptorSet*, uint32_t, const VkCopyDescriptorSet*)>(flextvkGetInstanceProcAddr(instance, "vkUpdateDescriptorSets"));
    flextvkWaitForFences = reinterpret_cast<VkResult(VKAPI_PTR*)(VkDevice, uint32_t, const VkFence*, VkBool32, uint64_t)>(flextvkGetInstanceProcAddr(instance, "vkWaitForFences"));
}
