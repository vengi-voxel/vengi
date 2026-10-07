/**
 * @file
 *
 * Some great tips here: https://developer.nvidia.com/opengl-vulkan
 */

#include "VkRenderer.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/StandardLib.h"
#include "core/Trace.h"
#include "core/collection/DynamicStringMap.h"
#include "flextVk.h"
#include "io/Filesystem.h"
#include "video/Renderer.h"
#include "video/RenderStats.h"
#include "video/RenderBuffer.h"
#include "video/ShaderTypes.h"
#include "video/Texture.h"
#include "video/TextureConfig.h"
#include "video/Types.h"
#include "video/vk/VkMapping.h"
#include <SDL3/SDL_vulkan.h>

// flextVk is generated from a 1.0 profile that does not include validation-layer
// pNext structs. Define the Khronos types locally so we can enable GPU-AV, sync
// validation and best practices without regenerating the loader.
#ifndef VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT
#define VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT ((VkStructureType)1000247000)
#endif
#ifndef VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT
#define VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT ((VkStructureType)1000496000)
#endif

#ifndef VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT
typedef enum VkValidationFeatureEnableEXT {
	VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT = 0,
	VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT = 1,
	VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT = 2,
	VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT = 3,
	VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT = 4
} VkValidationFeatureEnableEXT;
#endif

#ifndef VK_LAYER_SETTING_TYPE_BOOL32_EXT
typedef enum VkLayerSettingTypeEXT {
	VK_LAYER_SETTING_TYPE_BOOL32_EXT = 0,
	VK_LAYER_SETTING_TYPE_INT32_EXT = 1,
	VK_LAYER_SETTING_TYPE_INT64_EXT = 2,
	VK_LAYER_SETTING_TYPE_UINT32_EXT = 3,
	VK_LAYER_SETTING_TYPE_UINT64_EXT = 4,
	VK_LAYER_SETTING_TYPE_FLOAT32_EXT = 5,
	VK_LAYER_SETTING_TYPE_FLOAT64_EXT = 6,
	VK_LAYER_SETTING_TYPE_STRING_EXT = 7
} VkLayerSettingTypeEXT;

typedef struct VkLayerSettingEXT {
	const char *pLayerName;
	const char *pSettingName;
	VkLayerSettingTypeEXT type;
	uint32_t valueCount;
	const void *pValues;
} VkLayerSettingEXT;

typedef struct VkLayerSettingsCreateInfoEXT {
	VkStructureType sType;
	const void *pNext;
	uint32_t settingCount;
	const VkLayerSettingEXT *pSettings;
} VkLayerSettingsCreateInfoEXT;
#endif

#ifndef VK_VALIDATION_FEATURES_EXT_STRUCT
#define VK_VALIDATION_FEATURES_EXT_STRUCT
typedef struct VkValidationFeaturesEXT {
	VkStructureType sType;
	const void *pNext;
	uint32_t enabledValidationFeatureCount;
	const VkValidationFeatureEnableEXT *pEnabledValidationFeatures;
	uint32_t disabledValidationFeatureCount;
	const void *pDisabledValidationFeatures;
} VkValidationFeaturesEXT;
#endif

#ifndef VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 ((VkStructureType)1000059000)
#endif
#ifndef VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES ((VkStructureType)1000207000)
#endif
#ifndef VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES ((VkStructureType)1000211000)
#endif
#ifndef VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES ((VkStructureType)1000257000)
#endif
#ifndef VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES ((VkStructureType)1000177000)
#endif

#ifndef VK_PHYSICAL_DEVICE_FEATURES_2_STRUCT
#define VK_PHYSICAL_DEVICE_FEATURES_2_STRUCT
typedef struct VkPhysicalDeviceFeatures2 {
	VkStructureType sType;
	void *pNext;
	VkPhysicalDeviceFeatures features;
} VkPhysicalDeviceFeatures2;
typedef struct VkPhysicalDeviceTimelineSemaphoreFeatures {
	VkStructureType sType;
	void *pNext;
	VkBool32 timelineSemaphore;
} VkPhysicalDeviceTimelineSemaphoreFeatures;
typedef struct VkPhysicalDeviceVulkanMemoryModelFeatures {
	VkStructureType sType;
	void *pNext;
	VkBool32 vulkanMemoryModel;
	VkBool32 vulkanMemoryModelDeviceScope;
	VkBool32 vulkanMemoryModelAvailabilityVisibilityChains;
} VkPhysicalDeviceVulkanMemoryModelFeatures;
typedef struct VkPhysicalDeviceBufferDeviceAddressFeatures {
	VkStructureType sType;
	void *pNext;
	VkBool32 bufferDeviceAddress;
	VkBool32 bufferDeviceAddressCaptureReplay;
	VkBool32 bufferDeviceAddressMultiDevice;
} VkPhysicalDeviceBufferDeviceAddressFeatures;
typedef struct VkPhysicalDevice8BitStorageFeatures {
	VkStructureType sType;
	void *pNext;
	VkBool32 storageBuffer8BitAccess;
	VkBool32 uniformAndStorageBuffer8BitAccess;
	VkBool32 storagePushConstant8;
} VkPhysicalDevice8BitStorageFeatures;
typedef void(VKAPI_PTR *PFN_vkGetPhysicalDeviceFeatures2)(VkPhysicalDevice physicalDevice,
														  VkPhysicalDeviceFeatures2 *pFeatures);
#endif

namespace video {
namespace _priv {

// Forward declarations for rendering infrastructure (must match implementation)
static bool createRenderPass(VkDevice device, VkFormat colorFormat, VkFormat glowFormat, VkRenderPass &outRenderPass);
static bool createFramebuffers(VkDevice device, VkRenderPass renderPass, const VkImageView *imageViews,
							   const VkImageView *glowImageViews, uint32_t imageCount, VkExtent2D extent,
							   VkFramebuffer *&outFramebuffers);
static int32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeBits, VkMemoryPropertyFlags required,
							  VkMemoryPropertyFlags preferred = 0);

static const uint32_t MAX_FRAMES_IN_FLIGHT = 2u;

struct VkState : public RendererState {
	struct ShaderHandle {
		Id id = InvalidId;
		ShaderType type = ShaderType::Vertex;
		bool alive = false;
		VkShaderModule module = VK_NULL_HANDLE;
		core::String source;
	};
	struct ProgramHandle {
		Id id = InvalidId;
		bool alive = false;
		Id vertShader = InvalidId;
		Id fragShader = InvalidId;
		Id geomShader = InvalidId;
		Id computeShader = InvalidId;
		VkPipeline computePipeline = VK_NULL_HANDLE;
		// descriptor set layout derived from reflection (one binding per UBO)
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkPipelineLayout layout = VK_NULL_HANDLE;
		// Per-program pipeline cache keyed by vertex-input signature + topology.
		// A single program is typically drawn with several different VAOs during
		// the same frame (e.g. ShapeRenderer creates one VBO per shape); destroying
		// the currently-bound pipeline to rebuild for a new VAO would invalidate
		// the recording command buffer, so keep all variants alive for the life of
		// the program.
		struct PipelineEntry {
			uint64_t hash = 0u;
			// Render pass the pipeline was created against. Pipelines are only
			// reusable inside the *same* (or a render-pass-compatible) render
			// pass, so the cache must be keyed by it as well as by the
			// vertex-input signature. Off-screen FBOs use their own per-FBO
			// render passes, hence multiple entries per program.
			VkRenderPass renderPass = VK_NULL_HANDLE;
			bool hasDepth = false;
			VkPipeline pipeline = VK_NULL_HANDLE;
		};
		PipelineEntry *pipelines = nullptr;
		uint32_t pipelineCount = 0u;
		uint32_t pipelineCapacity = 0u;
		core::String name;
		core::DynamicStringMap<int32_t> uniformOffsets;
		ShaderUniforms uniforms;
		ShaderAttributes attributes;
		// Bindings registered by shadertool-generated code
		static const uint32_t MAX_BINDINGS = 16u;
		ShaderResourceBinding registeredBindings[MAX_BINDINGS] = {};
		uint32_t registeredBindingCount = 0u;
		uint32_t pushConstantSize = 0u;
	};
	struct VaoHandle {
		Id id = InvalidId;
		bool alive = false;
		// captured attribute descriptions (set by configureAttribute while this VAO is bound)
		Attribute attributes[16];
		uint32_t attributeCount = 0u;
		// per-bufferIndex ArrayBuffer handle snapshot (taken at configureAttribute time)
		Id arrayBuffers[16] = {};
		// index buffer associated with the VAO
		Id indexBuffer = InvalidId;
	};
	struct BufferHandle {
		Id id = InvalidId;
		bool alive = false;
		BufferType type = BufferType::Max;
		// Persistent host-malloc copy of the last uploaded data. Kept around so
		// unmapBuffer() has something to return and so backends without a GPU
		// mapping path can still fall back to it.
		uint8_t *data = nullptr;
		size_t size = 0u;
		// Actual VkBuffer + backing memory. Host-visible and persistently
		// mapped so bufferData()/bufferSubData() can memcpy directly.
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkDeviceSize capacity = 0u; // allocation size (may be larger than size)
		VkBufferUsageFlags usage = 0u;
		void *mapped = nullptr; // persistently mapped pointer (HOST_VISIBLE|HOST_COHERENT)
		bool slabMemory = false;
		// Partial update tracking. When only push-overlay fields change, the
		// uniform ring can reuse the previous snapshot (see setUniformBufferPushOverlay).
		bool partialDirty = false;
		size_t partialOffset = 0u;
		size_t partialSize = 0u;
		uint32_t stableRingFrame = ~0u;
		VkBuffer stableRingBuffer = VK_NULL_HANDLE;
		VkDeviceSize stableRingOffset = 0u;
		uint8_t *stableData = nullptr;
		size_t stableDataSize = 0u;
		static const uint32_t MAX_PUSH_OVERLAY_FIELDS = 8u;
		UniformPushOverlayField pushOverlayFields[MAX_PUSH_OVERLAY_FIELDS] = {};
		uint32_t pushOverlayFieldCount = 0u;
		uint32_t pushConstantSize = 0u;
	};
	struct TextureHandle {
		Id id = InvalidId;
		bool alive = false;
		TextureType type = TextureType::Texture2D;
		TextureFormat format = TextureFormat::RGBA;
		int width = 0;
		int height = 0;
		uint8_t layers = 1;
		bool isDepth = false;
		// VkImage + memory + view. View is created lazily once the format/size
		// is known (in uploadTexture). Sampler is shared per-config but for the
		// initial wiring we just create one per texture.
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;
		// Cached descriptor set used as ImTextureID by the ImGui Vulkan
		// backend so ImGui::Image(handle) works without further plumbing.
		// Allocated lazily on first use after the view is ready.
		VkDescriptorSet imguiSet = VK_NULL_HANDLE;
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		// True if uploadTexture has placed image contents on the GPU. False for
		// FBO color attachments before they are first rendered into.
		bool hasContent = false;
		bool storageBit = false;
		bool slabMemory = false;
	};
	struct FramebufferHandle {
		Id id = InvalidId;
		bool alive = false;
		// Color attachments + optional depth attachment by texture id.
		Id colorAttachments[8] = {};
		Id depthAttachment = InvalidId;
		uint32_t colorCount = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		bool dirty = true; // attachments changed -> rebuild renderPass+framebuffer
		VkRenderPass renderPass = VK_NULL_HANDLE;
		VkRenderPass renderPassLoad = VK_NULL_HANDLE; // LOAD_OP_LOAD variant for re-entries
		VkRenderPass renderPassClearDefined = VK_NULL_HANDLE; // LOAD_OP_CLEAR with defined layouts
		VkRenderPass renderPassColorClear = VK_NULL_HANDLE;	  // color CLEAR (undefined), depth LOAD
		VkRenderPass renderPassColorClearDefined = VK_NULL_HANDLE; // color CLEAR (defined), depth LOAD
		VkFramebuffer framebuffer = VK_NULL_HANDLE;
		VkImageView depthLayerView = VK_NULL_HANDLE;
		int depthLayer = 0;
		// Cached attachment formats so the pipeline cache can key on them.
		VkFormat colorFormats[8] = {};
		VkFormat depthFormat = VK_FORMAT_UNDEFINED;
	};

	// Core Vulkan handles
	VkInstance instance = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkDebugReportCallbackEXT debugReportCallback = VK_NULL_HANDLE;
	bool validationEnabled = false;
	DebugSeverity debugSeverity = DebugSeverity::Medium;
	SDL_Window *window = nullptr;
	int windowWidth = 0;
	int windowHeight = 0;
	float scaleFactor = 1.0f;
	uint32_t graphicsQueueFamily = UINT32_MAX;
	uint32_t presentQueueFamily = UINT32_MAX;

	// Command pool and single utility command buffer
	VkCommandPool commandPool = VK_NULL_HANDLE;
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkCommandPool frameCommandPools[MAX_FRAMES_IN_FLIGHT] = {};

	// Sub-allocate small images/buffers from these. Allocations larger than
	// MEMORY_SLAB_SIZE (typical viewport RGBA16F / shadow arrays) use dedicated
	// VkDeviceMemory on the handle so they can be freed on destroy.
	static const VkDeviceSize MEMORY_SLAB_SIZE = 64u * 1024u * 1024u;
	struct MemorySlab {
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkDeviceSize capacity = 0;
		VkDeviceSize used = 0;
		uint32_t memoryTypeIndex = UINT32_MAX;
		VkMemoryPropertyFlags properties = 0;
		void *mapped = nullptr;
	};
	MemorySlab *memorySlabs = nullptr;
	uint32_t memorySlabCount = 0u;
	uint32_t memorySlabCapacity = 0u;

	// Swapchain
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkFormat swapchainFormat = (VkFormat)0;
	VkExtent2D swapchainExtent = {0, 0};
	uint32_t swapchainImageCount = 0;
	uint32_t swapchainMinImageCount = 0;
	VkImage *swapchainImages = nullptr;
	VkImageView *swapchainImageViews = nullptr;

	// Secondary color attachment used by the main subpass to absorb writes to
	// fragment-shader output location 1 (`o_glow`). Without this attachment the
	// validation layers warn on every pipeline build that a declared shader
	// output has no matching color attachment. One image/view/memory per
	// swapchain image so the attachment matches the framebuffer lifetime.
	VkFormat glowFormat = VK_FORMAT_R8G8B8A8_UNORM;
	VkImage *glowImages = nullptr;
	VkImageView *glowImageViews = nullptr;
	VkDeviceMemory *glowMemory = nullptr;

	// Per-frame-in-flight resources
	VkCommandBuffer frameCommandBuffers[MAX_FRAMES_IN_FLIGHT] = {};
	VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT] = {};
	// One renderFinished semaphore per swapchain image (signaled by submit,
	// waited on by vkQueuePresentKHR). Sizing it per-image avoids the classic
	// validation warning when the acquire order differs from the frame order.
	VkSemaphore *renderFinishedSemaphores = nullptr;
	uint32_t renderFinishedSemaphoreCount = 0;
	VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT] = {};
	// True after vkQueueSubmit used this slot's fence, until vkResetFences.
	// waitAllInFlightFrames must not wait a reset-but-never-submitted fence
	// (cleanup / screenshot-quit skip endFrame and would block forever).
	bool inFlightFencePending[MAX_FRAMES_IN_FLIGHT] = {};
	uint32_t currentFrame = 0;
	uint32_t currentImageIndex = 0;
	bool frameAcquired = false;
	// Tracks whether vkCmdNextSubpass has already advanced the command buffer
	// into the overlay subpass for the current frame; endFrame() guarantees it
	// is called at least once before vkCmdEndRenderPass so the render pass
	// completes all of its declared subpasses (Vulkan spec requirement).
	bool advancedToOverlaySubpass = false;

	// Render infrastructure
	VkRenderPass renderPass = VK_NULL_HANDLE;
	// LOAD_OP_LOAD variant of the main render pass, used to resume after an
	// off-screen FBO render pass within the same frame.
	VkRenderPass mainResumeRenderPass = VK_NULL_HANDLE;
	VkFramebuffer *framebuffers = nullptr;

	// Descriptor pool shared with ImGui (and, later, the rest of the engine)
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	// 1x1 RGBA placeholder used to satisfy combined-image-sampler descriptor
	// bindings before the engine's texture upload path is wired through to
	// real VkImages. Without it, every draw with a sampler in its layout
	// triggers VK_ERROR validation about unbound descriptors.
	VkImage placeholderImage = VK_NULL_HANDLE;
	VkDeviceMemory placeholderImageMemory = VK_NULL_HANDLE;
	VkImageView placeholderImageView = VK_NULL_HANDLE;
	VkImage placeholderDepthArrayImage = VK_NULL_HANDLE;
	VkDeviceMemory placeholderDepthArrayMemory = VK_NULL_HANDLE;
	VkImageView placeholderDepthArrayView = VK_NULL_HANDLE;
	VkSampler placeholderDepthArraySampler = VK_NULL_HANDLE;
	VkSampler placeholderSampler = VK_NULL_HANDLE;
	// Transient descriptor sets allocated per draw and freed together when the
	// owning frame slot is reused next time around the swapchain. Keeps each
	// draw's descriptor set immutable after binding, which is what Vulkan
	// requires (vkUpdateDescriptorSets on a bound set invalidates the cmd buf).
	VkDescriptorSet *pendingFreeSets[MAX_FRAMES_IN_FLIGHT] = {};
	uint32_t pendingFreeSetCount[MAX_FRAMES_IN_FLIGHT] = {};
	uint32_t pendingFreeSetCapacity[MAX_FRAMES_IN_FLIGHT] = {};
	// Off-screen VkFramebuffer/VkRenderPass objects queued for destroy once every
	// in-flight frame slot has finished (they may still be referenced by a
	// recording or submitted command buffer until then).
	struct PendingFboDestroy {
		VkFramebuffer framebuffer = VK_NULL_HANDLE;
		VkRenderPass renderPass = VK_NULL_HANDLE;
		VkRenderPass renderPassLoad = VK_NULL_HANDLE;
		VkRenderPass renderPassClearDefined = VK_NULL_HANDLE;
		VkRenderPass renderPassColorClear = VK_NULL_HANDLE;
		VkRenderPass renderPassColorClearDefined = VK_NULL_HANDLE;
		VkImageView extraView = VK_NULL_HANDLE;
	};
	PendingFboDestroy *pendingFboDestroys = nullptr;
	uint32_t pendingFboDestroyCount = 0u;
	uint32_t pendingFboDestroyCapacity = 0u;
	// Buffers replaced/grown while a command buffer may still reference the old
	// VkBuffer (ensureBufferAlloc). Destroy after in-flight frames complete.
	struct PendingBufferDestroy {
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		void *mapped = nullptr;
		bool freeMemory = false;
	};
	PendingBufferDestroy *pendingBufferDestroys = nullptr;
	uint32_t pendingBufferDestroyCount = 0u;
	uint32_t pendingBufferDestroyCapacity = 0u;
	// Per-frame rings of uniform data snapshotted at each draw. Without this, all
	// draws in a command buffer read the last CPU write to each shared UBO.
	// Chunks grow if a frame needs more than UNIFORM_RING_CHUNK_SIZE.
	static const VkDeviceSize UNIFORM_RING_CHUNK_SIZE = 4u * 1024u * 1024u;
	static const uint32_t UNIFORM_RING_MAX_CHUNKS = 32u;
	struct UniformRingChunk {
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		void *mapped = nullptr;
		VkDeviceSize capacity = 0u;
		VkDeviceSize offset = 0u;
	};
	struct UniformRingFrame {
		UniformRingChunk *chunks = nullptr;
		uint32_t chunkCount = 0u;
		uint32_t chunkCapacity = 0u;
		uint32_t currentChunk = 0u;
	};
	UniformRingFrame uniformRings[MAX_FRAMES_IN_FLIGHT];
	VkDeviceSize uniformRingAlignment = 256u;

	// Opaque renderer handle model for incremental backend bring-up
	Id nextHandleId = 1u;
	Id currentFramebuffer = InvalidId;
	ShaderHandle *shaderHandles = nullptr;
	uint32_t shaderHandleCount = 0;
	uint32_t shaderHandleCapacity = 0;
	ProgramHandle *programHandles = nullptr;
	uint32_t programHandleCount = 0;
	uint32_t programHandleCapacity = 0;
	BufferHandle *bufferHandles = nullptr;
	uint32_t bufferHandleCount = 0;
	uint32_t bufferHandleCapacity = 0;
	// O(1) Id -> bufferHandles[] index (UINT32_MAX = missing). Required: MCA scenes
	// create thousands of buffers and findBufferHandle is on the draw hot path.
	uint32_t *bufferIdIndex = nullptr;
	uint32_t bufferIdIndexCapacity = 0u;
	VaoHandle *vaoHandles = nullptr;
	uint32_t vaoHandleCount = 0;
	uint32_t vaoHandleCapacity = 0;
	uint32_t *vaoIdIndex = nullptr;
	uint32_t vaoIdIndexCapacity = 0u;
	TextureHandle *textureHandles = nullptr;
	uint32_t textureHandleCount = 0;
	uint32_t textureHandleCapacity = 0;
	FramebufferHandle *framebufferHandles = nullptr;
	uint32_t framebufferHandleCount = 0;
	uint32_t framebufferHandleCapacity = 0;
	// Track which FBO is currently bound and the active render pass state so
	// bindFramebuffer() can switch render passes mid-frame.
	Id boundFboHandle = InvalidId;
	bool mainRenderPassActive = false;
	bool fboRenderPassActive = false;
	// Render pass currently recorded into. Used by findOrCreatePipeline to
	// pick (or build) a pipeline that's render-pass-compatible with the
	// active vkCmdBeginRenderPass.
	VkRenderPass currentRenderPass = VK_NULL_HANDLE;
	uint32_t currentColorAttachmentCount = 0u;
	bool currentRenderPassHasDepth = false;
	uint8_t drawBufferMask = 0xFFu;
	// Per-texture-unit currently bound texture. Picked up by acquireDescriptorSet
	// to wire combined image samplers in shaders to actual VkImages.
	Id boundTextures[16] = {};

	// Draw-state cache for the currently recording command buffer. Avoids
	// per-draw vkAllocateDescriptorSets / vkCmdBindPipeline when nothing changed
	// except UBO contents (handled via UNIFORM_BUFFER_DYNAMIC offsets).
	VkPipeline boundGraphicsPipeline = VK_NULL_HANDLE;
	Id boundVaoId = InvalidId;
	Id descCacheProgramId = InvalidId;
	VkDescriptorSet descCacheSet = VK_NULL_HANDLE;
	Id descCacheTextures[16] = {};
	Id descCacheImage = InvalidId;
	VkBuffer descCacheUboBuffers[16] = {};
	VkDeviceSize descCacheUboRanges[16] = {};
	uint32_t descCacheUboCount = 0u;
	// Last dynamic offsets actually bound on the recording command buffer.
	// When the descriptor set is reused and offsets are unchanged (stable VertData
	// ring slot), skip vkCmdBindDescriptorSets entirely.
	uint32_t descBoundDynOffsets[16] = {};
	uint32_t descBoundDynOffsetCount = 0u;
	VkDescriptorSet descBoundSet = VK_NULL_HANDLE;
	int descCacheViewport[4] = {0, 0, -1, -1};
	int descCacheScissor[4] = {0, 0, -1, -1};
	// Current draw state
	Id currentProgram = InvalidId;
	VkClearColorValue clearColor = {{0.0f, 0.0f, 0.0f, 1.0f}};
};

static const char *vkResultStr(VkResult result) {
	switch (result) {
	case VK_SUCCESS:
		return "VK_SUCCESS";
	case VK_NOT_READY:
		return "VK_NOT_READY";
	case VK_TIMEOUT:
		return "VK_TIMEOUT";
	case VK_EVENT_SET:
		return "VK_EVENT_SET";
	case VK_EVENT_RESET:
		return "VK_EVENT_RESET";
	case VK_INCOMPLETE:
		return "VK_INCOMPLETE";
	case VK_ERROR_OUT_OF_HOST_MEMORY:
		return "VK_ERROR_OUT_OF_HOST_MEMORY";
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
	case VK_ERROR_INITIALIZATION_FAILED:
		return "VK_ERROR_INITIALIZATION_FAILED";
	case VK_ERROR_DEVICE_LOST:
		return "VK_ERROR_DEVICE_LOST";
	case VK_ERROR_MEMORY_MAP_FAILED:
		return "VK_ERROR_MEMORY_MAP_FAILED";
	case VK_ERROR_LAYER_NOT_PRESENT:
		return "VK_ERROR_LAYER_NOT_PRESENT";
	case VK_ERROR_EXTENSION_NOT_PRESENT:
		return "VK_ERROR_EXTENSION_NOT_PRESENT";
	case VK_ERROR_FEATURE_NOT_PRESENT:
		return "VK_ERROR_FEATURE_NOT_PRESENT";
	case VK_ERROR_INCOMPATIBLE_DRIVER:
		return "VK_ERROR_INCOMPATIBLE_DRIVER";
	case VK_ERROR_TOO_MANY_OBJECTS:
		return "VK_ERROR_TOO_MANY_OBJECTS";
	case VK_ERROR_FORMAT_NOT_SUPPORTED:
		return "VK_ERROR_FORMAT_NOT_SUPPORTED";
	case VK_ERROR_SURFACE_LOST_KHR:
		return "VK_ERROR_SURFACE_LOST_KHR";
	case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
		return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
	case VK_SUBOPTIMAL_KHR:
		return "VK_SUBOPTIMAL_KHR";
	case VK_ERROR_OUT_OF_DATE_KHR:
		return "VK_ERROR_OUT_OF_DATE_KHR";
	default:
		return "VK_UNKNOWN";
	}
}

static Id allocHandleId(VkState &s) {
	Id id = s.nextHandleId++;
	if (id == InvalidId) {
		id = s.nextHandleId++;
	}	return id;
}

static VkState::ShaderHandle *findShaderHandle(VkState &s, Id id) {
	for (uint32_t i = 0; i < s.shaderHandleCount; ++i) {
		VkState::ShaderHandle &h = s.shaderHandles[i];
		if (h.alive && h.id == id) {
			return &h;
		}
	}
	return nullptr;
}

static VkState::ProgramHandle *findProgramHandle(VkState &s, Id id) {
	for (uint32_t i = 0; i < s.programHandleCount; ++i) {
		VkState::ProgramHandle &h = s.programHandles[i];
		if (h.alive && h.id == id) {
			return &h;
		}
	}
	return nullptr;
}

static bool ensureShaderCapacity(VkState &s) {
	if (s.shaderHandleCount < s.shaderHandleCapacity) {
		return true;
	}
	const uint32_t oldCapacity = s.shaderHandleCapacity;
	const uint32_t newCapacity = oldCapacity == 0u ? 64u : oldCapacity * 2u;
	VkState::ShaderHandle *newHandles = new VkState::ShaderHandle[newCapacity];
	if (newHandles == nullptr) {
		return false;
	}
	if (s.shaderHandles != nullptr) {
		for (uint32_t i = 0; i < oldCapacity; ++i) {
			newHandles[i] = core::move(s.shaderHandles[i]);
		}
		delete[] s.shaderHandles;
	}
	s.shaderHandles = newHandles;
	s.shaderHandleCapacity = newCapacity;
	return true;
}

static bool ensureProgramCapacity(VkState &s) {
	if (s.programHandleCount < s.programHandleCapacity) {
		return true;
	}
	const uint32_t oldCapacity = s.programHandleCapacity;
	const uint32_t newCapacity = oldCapacity == 0u ? 32u : oldCapacity * 2u;
	VkState::ProgramHandle *newHandles = new VkState::ProgramHandle[newCapacity];
	if (newHandles == nullptr) {
		return false;
	}
	if (s.programHandles != nullptr) {
		for (uint32_t i = 0; i < oldCapacity; ++i) {
			newHandles[i] = core::move(s.programHandles[i]);
		}
		delete[] s.programHandles;
	}
	s.programHandles = newHandles;
	s.programHandleCapacity = newCapacity;
	return true;
}

static bool ensureIdIndexCapacity(uint32_t **table, uint32_t *capacity, Id id) {
	if (id == InvalidId) {
		return false;
	}
	if ((uint32_t)id < *capacity) {
		return true;
	}
	uint32_t newCap = *capacity == 0u ? 1024u : *capacity;
	while (newCap <= (uint32_t)id) {
		newCap *= 2u;
	}
	uint32_t *neu = (uint32_t *)core_malloc(sizeof(uint32_t) * newCap);
	if (neu == nullptr) {
		return false;
	}
	for (uint32_t i = 0; i < newCap; ++i) {
		neu[i] = ~0u;
	}
	if (*table != nullptr) {
		core_memcpy(neu, *table, sizeof(uint32_t) * (*capacity));
		core_free(*table);
	}
	*table = neu;
	*capacity = newCap;
	return true;
}

static VkState::BufferHandle *findBufferHandle(VkState &s, Id id) {
	if (id == InvalidId || (uint32_t)id >= s.bufferIdIndexCapacity) {
		return nullptr;
	}
	const uint32_t idx = s.bufferIdIndex[id];
	if (idx == ~0u || idx >= s.bufferHandleCount) {
		return nullptr;
	}
	VkState::BufferHandle &h = s.bufferHandles[idx];
	if (!h.alive || h.id != id) {
		return nullptr;
	}
	return &h;
}

static bool ensureBufferCapacity(VkState &s) {
	if (s.bufferHandleCount < s.bufferHandleCapacity) {
		return true;
	}
	const uint32_t oldCapacity = s.bufferHandleCapacity;
	const uint32_t newCapacity = oldCapacity == 0u ? 64u : oldCapacity * 2u;
	VkState::BufferHandle *newHandles = new VkState::BufferHandle[newCapacity];
	if (newHandles == nullptr) {
		return false;
	}
	if (s.bufferHandles != nullptr) {
		for (uint32_t i = 0; i < oldCapacity; ++i) {
			newHandles[i] = core::move(s.bufferHandles[i]);
		}
		delete[] s.bufferHandles;
	}
	s.bufferHandles = newHandles;
	s.bufferHandleCapacity = newCapacity;
	return true;
}

static VkState::VaoHandle *findVaoHandle(VkState &s, Id id) {
	if (id == InvalidId || (uint32_t)id >= s.vaoIdIndexCapacity) {
		return nullptr;
	}
	const uint32_t idx = s.vaoIdIndex[id];
	if (idx == ~0u || idx >= s.vaoHandleCount) {
		return nullptr;
	}
	VkState::VaoHandle &h = s.vaoHandles[idx];
	if (!h.alive || h.id != id) {
		return nullptr;
	}
	return &h;
}

static bool ensureVaoCapacity(VkState &s) {
	if (s.vaoHandleCount < s.vaoHandleCapacity) {
		return true;
	}
	const uint32_t oldCapacity = s.vaoHandleCapacity;
	const uint32_t newCapacity = oldCapacity == 0u ? 32u : oldCapacity * 2u;
	VkState::VaoHandle *newHandles = new VkState::VaoHandle[newCapacity];
	if (newHandles == nullptr) {
		return false;
	}
	if (s.vaoHandles != nullptr) {
		for (uint32_t i = 0; i < oldCapacity; ++i) {
			newHandles[i] = core::move(s.vaoHandles[i]);
		}
		delete[] s.vaoHandles;
	}
	s.vaoHandles = newHandles;
	s.vaoHandleCapacity = newCapacity;
	return true;
}

static VkState::TextureHandle *findTextureHandle(VkState &s, Id id) {
	for (uint32_t i = 0; i < s.textureHandleCount; ++i) {
		VkState::TextureHandle &h = s.textureHandles[i];
		if (h.alive && h.id == id) {
			return &h;
		}
	}
	return nullptr;
}

static bool ensureTextureCapacity(VkState &s) {
	if (s.textureHandleCount < s.textureHandleCapacity) {
		return true;
	}
	const uint32_t oldCapacity = s.textureHandleCapacity;
	const uint32_t newCapacity = oldCapacity == 0u ? 64u : oldCapacity * 2u;
	VkState::TextureHandle *newHandles = new VkState::TextureHandle[newCapacity];
	if (newHandles == nullptr) {
		return false;
	}
	if (s.textureHandles != nullptr) {
		for (uint32_t i = 0; i < oldCapacity; ++i) {
			newHandles[i] = core::move(s.textureHandles[i]);
		}
		delete[] s.textureHandles;
	}
	s.textureHandles = newHandles;
	s.textureHandleCapacity = newCapacity;
	return true;
}

static VkFormat textureFormatToVk(TextureFormat fmt) {
	switch (fmt) {
	case TextureFormat::RGBA:
		return VK_FORMAT_R8G8B8A8_UNORM;
	case TextureFormat::RGB:
		return VK_FORMAT_R8G8B8_UNORM;
	case TextureFormat::RGBA32F:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case TextureFormat::RGB32F:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case TextureFormat::RGBA16F:
		return VK_FORMAT_R16G16B16A16_SFLOAT;
	case TextureFormat::D24S8:
		return VK_FORMAT_D24_UNORM_S8_UINT;
	case TextureFormat::D32FS8:
		return VK_FORMAT_D32_SFLOAT_S8_UINT;
	case TextureFormat::D24:
		return VK_FORMAT_X8_D24_UNORM_PACK32;
	case TextureFormat::D32F:
		return VK_FORMAT_D32_SFLOAT;
	case TextureFormat::S8:
		return VK_FORMAT_S8_UINT;
	case TextureFormat::RG16U:
		return VK_FORMAT_R16G16_UNORM;
	case TextureFormat::Max:
		break;
	}
	return VK_FORMAT_R8G8B8A8_UNORM;
}

static uint32_t textureFormatBytesPerPixel(TextureFormat fmt) {
	switch (fmt) {
	case TextureFormat::RGBA:
		return 4;
	case TextureFormat::RGB:
		return 3;
	case TextureFormat::RGBA32F:
		return 16;
	case TextureFormat::RGB32F:
		return 12;
	case TextureFormat::RGBA16F:
		return 8;
	case TextureFormat::D24S8:
	case TextureFormat::D32FS8:
	case TextureFormat::D24:
	case TextureFormat::D32F:
		return 4;
	case TextureFormat::S8:
		return 1;
	case TextureFormat::RG16U:
		return 4;
	case TextureFormat::Max:
		break;
	}
	return 4;
}

static bool textureFormatIsDepth(TextureFormat fmt) {
	switch (fmt) {
	case TextureFormat::D24S8:
	case TextureFormat::D32FS8:
	case TextureFormat::D24:
	case TextureFormat::D32F:
	case TextureFormat::S8:
		return true;
	default:
		return false;
	}
}

static bool textureFormatHasStencil(TextureFormat fmt) {
	return fmt == TextureFormat::D24S8 || fmt == TextureFormat::D32FS8 || fmt == TextureFormat::S8;
}

static VkImageAspectFlags imageAspectForFormat(TextureFormat fmt) {
	if (fmt == TextureFormat::S8) {
		return VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	if (textureFormatIsDepth(fmt)) {
		return textureFormatHasStencil(fmt)
				   ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)
				   : VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	return VK_IMAGE_ASPECT_COLOR_BIT;
}

enum class FboBeginClear : uint8_t { Auto = 0, All = 1, Color = 2 };

static VkState::FramebufferHandle *findFramebufferHandle(VkState &s, Id id) {
	for (uint32_t i = 0; i < s.framebufferHandleCount; ++i) {
		VkState::FramebufferHandle &h = s.framebufferHandles[i];
		if (h.alive && h.id == id) {
			return &h;
		}
	}
	return nullptr;
}

static bool ensureFramebufferCapacity(VkState &s) {
	if (s.framebufferHandleCount < s.framebufferHandleCapacity) {
		return true;
	}
	const uint32_t oldCapacity = s.framebufferHandleCapacity;
	const uint32_t newCapacity = oldCapacity == 0u ? 16u : oldCapacity * 2u;
	VkState::FramebufferHandle *newHandles = new VkState::FramebufferHandle[newCapacity];
	if (newHandles == nullptr) {
		return false;
	}
	if (s.framebufferHandles != nullptr) {
		for (uint32_t i = 0; i < oldCapacity; ++i) {
			newHandles[i] = core::move(s.framebufferHandles[i]);
		}
		delete[] s.framebufferHandles;
	}
	s.framebufferHandles = newHandles;
	s.framebufferHandleCapacity = newCapacity;
	return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugReportCallback(VkDebugReportFlagsEXT flags,
															VkDebugReportObjectTypeEXT objectType, uint64_t object,
															size_t location, int32_t messageCode,
															const char *layerPrefix, const char *message,
															void *userData) {
	const VkState *state = (const VkState *)userData;
	const DebugSeverity severity = state != nullptr ? state->debugSeverity : DebugSeverity::Medium;
	if (severity == DebugSeverity::None) {
		return VK_FALSE;
	}
	(void)objectType;
	(void)object;
	(void)location;
	const bool isError = (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) != 0;
	const bool isWarn = (flags & (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)) != 0;
	if (isError) {
		Log::error("Vulkan [%s] (%d): %s", layerPrefix, messageCode, message);
	} else if (isWarn && severity != DebugSeverity::High) {
		Log::warn("Vulkan [%s] (%d): %s", layerPrefix, messageCode, message);
	} else if (severity == DebugSeverity::Low) {
		Log::debug("Vulkan [%s] (%d): %s", layerPrefix, messageCode, message);
	}
	return VK_FALSE;
}

static bool hasInstanceExtension(const char *extensionName) {
	uint32_t extCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	if (extCount == 0) {
		return false;
	}
	VkExtensionProperties *exts = (VkExtensionProperties *)core_malloc(sizeof(VkExtensionProperties) * extCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, exts);
	for (uint32_t i = 0; i < extCount; ++i) {
		if (SDL_strcmp(exts[i].extensionName, extensionName) == 0) {
			core_free((void *)exts);
			return true;
		}
	}
	core_free((void *)exts);
	return false;
}

static bool hasInstanceLayer(const char *layerName) {
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	if (layerCount == 0) {
		return false;
	}
	VkLayerProperties *layers = (VkLayerProperties *)core_malloc(sizeof(VkLayerProperties) * layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, layers);
	for (uint32_t i = 0; i < layerCount; ++i) {
		if (SDL_strcmp(layers[i].layerName, layerName) == 0) {
			core_free((void *)layers);
			return true;
		}
	}
	core_free((void *)layers);
	return false;
}

static void destroyBufferAlloc(VkState &s, VkState::BufferHandle &h);
static bool dedicatedAlloc(VkState &s, VkDeviceSize size, uint32_t memoryTypeBits, VkMemoryPropertyFlags properties,
						   VkDeviceMemory *outMemory, void **outMapped, VkMemoryPropertyFlags preferred = 0);
static bool slabAlloc(VkState &s, VkDeviceSize size, VkDeviceSize alignment, uint32_t memoryTypeBits,
					  VkMemoryPropertyFlags properties, VkDeviceMemory *outMemory, VkDeviceSize *outOffset,
					  void **outMapped, VkMemoryPropertyFlags preferred = 0);
static bool allocGpuMemory(VkState &s, VkDeviceSize size, VkDeviceSize alignment, uint32_t memoryTypeBits,
						   VkMemoryPropertyFlags properties, VkDeviceMemory *outMemory, VkDeviceSize *outOffset,
						   void **outMapped, bool *outSlab, VkMemoryPropertyFlags preferred = 0);
static void destroyMemorySlabs(VkState &s);

// Destroy swapchain images/views and sync objects, but keep the swapchain handle itself
// for potential recreation. Pass destroySwapchainHandle=true to also destroy the handle.
static void destroySwapchainResources(VkState &s, bool destroySwapchainHandle) {
	if (s.device == VK_NULL_HANDLE) {
		return;
	}

	// Wait for device idle before destroying resources
	vkDeviceWaitIdle(s.device);

	// Destroy per-frame sync objects (acquire semaphores + fences)
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		if (s.inFlightFences[i] != VK_NULL_HANDLE) {
			vkDestroyFence(s.device, s.inFlightFences[i], nullptr);
			s.inFlightFences[i] = VK_NULL_HANDLE;
		}
		s.inFlightFencePending[i] = false;
		if (s.imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
			vkDestroySemaphore(s.device, s.imageAvailableSemaphores[i], nullptr);
			s.imageAvailableSemaphores[i] = VK_NULL_HANDLE;
		}
	}
	// Destroy per-image render-finished semaphores
	if (s.renderFinishedSemaphores != nullptr) {
		for (uint32_t i = 0; i < s.renderFinishedSemaphoreCount; ++i) {
			if (s.renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
				vkDestroySemaphore(s.device, s.renderFinishedSemaphores[i], nullptr);
			}
		}
		core_free((void *)s.renderFinishedSemaphores);
		s.renderFinishedSemaphores = nullptr;
	}
	s.renderFinishedSemaphoreCount = 0;
	s.frameAcquired = false;
	s.currentFrame = 0;

	// Free per-frame command buffers (from per-frame command pools)
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		if (s.frameCommandBuffers[i] != VK_NULL_HANDLE && s.frameCommandPools[i] != VK_NULL_HANDLE) {
			vkFreeCommandBuffers(s.device, s.frameCommandPools[i], 1, &s.frameCommandBuffers[i]);
			s.frameCommandBuffers[i] = VK_NULL_HANDLE;
		}
	}

	// Destroy framebuffers (they reference the image views being destroyed below)
	if (s.framebuffers != nullptr) {
		for (uint32_t i = 0; i < s.swapchainImageCount; ++i) {
			if (s.framebuffers[i] != VK_NULL_HANDLE) {
				vkDestroyFramebuffer(s.device, s.framebuffers[i], nullptr);
			}
		}
		core_free((void *)s.framebuffers);
		s.framebuffers = nullptr;
	}

	// Destroy image views
	if (s.swapchainImageViews != nullptr) {
		for (uint32_t i = 0; i < s.swapchainImageCount; ++i) {
			if (s.swapchainImageViews[i] != VK_NULL_HANDLE) {
				vkDestroyImageView(s.device, s.swapchainImageViews[i], nullptr);
			}
		}
		core_free((void *)s.swapchainImageViews);
		s.swapchainImageViews = nullptr;
	}

	// Destroy glow attachments (image view + image + memory per swapchain image)
	if (s.glowImageViews != nullptr) {
		for (uint32_t i = 0; i < s.swapchainImageCount; ++i) {
			if (s.glowImageViews[i] != VK_NULL_HANDLE) {
				vkDestroyImageView(s.device, s.glowImageViews[i], nullptr);
			}
		}
		core_free((void *)s.glowImageViews);
		s.glowImageViews = nullptr;
	}
	if (s.glowImages != nullptr) {
		for (uint32_t i = 0; i < s.swapchainImageCount; ++i) {
			if (s.glowImages[i] != VK_NULL_HANDLE) {
				vkDestroyImage(s.device, s.glowImages[i], nullptr);
			}
		}
		core_free((void *)s.glowImages);
		s.glowImages = nullptr;
	}
	if (s.glowMemory != nullptr) {
		for (uint32_t i = 0; i < s.swapchainImageCount; ++i) {
			if (s.glowMemory[i] != VK_NULL_HANDLE) {
				vkFreeMemory(s.device, s.glowMemory[i], nullptr);
			}
		}
		core_free((void *)s.glowMemory);
		s.glowMemory = nullptr;
	}

	if (s.swapchainImages != nullptr) {
		core_free((void *)s.swapchainImages);
		s.swapchainImages = nullptr;
	}
	s.swapchainImageCount = 0;

	if (destroySwapchainHandle && s.swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(s.device, s.swapchain, nullptr);
		s.swapchain = VK_NULL_HANDLE;
	}
}

static bool createSwapchain(VkState &s) {
	// Query surface capabilities
	VkSurfaceCapabilitiesKHR caps;
	VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(s.physicalDevice, s.surface, &caps);
	if (result != VK_SUCCESS) {
		Log::error("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed: %s", vkResultStr(result));
		return false;
	}

	// Choose surface format. We prefer a non-sRGB (UNORM) swapchain to match the default
	// OpenGL backbuffer behavior used throughout the engine: shaders and ImGui vertex
	// colors are authored in sRGB space and written directly to the framebuffer without
	// a hardware gamma conversion. Picking a *_SRGB swapchain format would re-encode
	// those already-sRGB values and produce the classic "too bright" look.
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(s.physicalDevice, s.surface, &formatCount, nullptr);
	VkSurfaceFormatKHR *formats = (VkSurfaceFormatKHR *)core_malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(s.physicalDevice, s.surface, &formatCount, formats);

	VkSurfaceFormatKHR chosenFormat = formats[0];
	bool foundPreferred = false;
	const VkFormat preferred[] = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM};
	for (uint32_t p = 0; p < lengthof(preferred) && !foundPreferred; ++p) {
		for (uint32_t i = 0; i < formatCount; ++i) {
			if (formats[i].format == preferred[p] && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				chosenFormat = formats[i];
				foundPreferred = true;
				break;
			}
		}
	}
	core_free(formats);

	// Choose present mode (prefer MAILBOX for low latency, fallback to FIFO)
	uint32_t presentModeCount = 0;
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(s.physicalDevice, s.surface, &presentModeCount, nullptr);
	if (result != VK_SUCCESS || presentModeCount == 0) {
		Log::error("vkGetPhysicalDeviceSurfacePresentModesKHR (count) failed: %s", vkResultStr(result));
		return false;
	}
	uint32_t *presentModesRaw = (uint32_t *)core_malloc(sizeof(uint32_t) * presentModeCount);
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(s.physicalDevice, s.surface, &presentModeCount,
													   (VkPresentModeKHR *)presentModesRaw);
	if (result != VK_SUCCESS) {
		Log::error("vkGetPhysicalDeviceSurfacePresentModesKHR (values) failed: %s", vkResultStr(result));
		core_free((void *)presentModesRaw);
		return false;
	}

	uint32_t chosenPresentModeRaw = (uint32_t)VK_PRESENT_MODE_FIFO_KHR;
	for (uint32_t i = 0; i < presentModeCount; ++i) {
		if (presentModesRaw[i] == (uint32_t)VK_PRESENT_MODE_MAILBOX_KHR) {
			chosenPresentModeRaw = (uint32_t)VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
	}
	core_free((void *)presentModesRaw);
	const VkPresentModeKHR chosenPresentMode = (VkPresentModeKHR)chosenPresentModeRaw;

	// Choose swapchain extent
	VkExtent2D extent;
	if (caps.currentExtent.width != UINT32_MAX) {
		extent = caps.currentExtent;
	} else {
		uint32_t w = (uint32_t)s.windowWidth;
		uint32_t h = (uint32_t)s.windowHeight;
		extent.width = w < caps.minImageExtent.width ? caps.minImageExtent.width
													 : (w > caps.maxImageExtent.width ? caps.maxImageExtent.width : w);
		extent.height = h < caps.minImageExtent.height
							? caps.minImageExtent.height
							: (h > caps.maxImageExtent.height ? caps.maxImageExtent.height : h);
	}

	// Choose image count (prefer min+1 for triple buffering if supported)
	uint32_t imageCount = caps.minImageCount + 1;
	if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
		imageCount = caps.maxImageCount;
	}

	VkSwapchainKHR oldSwapchain = s.swapchain;

	uint32_t queueFamilyIndices[2] = {s.graphicsQueueFamily, s.presentQueueFamily};
	VkSwapchainCreateInfoKHR swapchainInfo;
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.pNext = nullptr;
	swapchainInfo.flags = 0;
	swapchainInfo.surface = s.surface;
	swapchainInfo.minImageCount = imageCount;
	swapchainInfo.imageFormat = chosenFormat.format;
	swapchainInfo.imageColorSpace = chosenFormat.colorSpace;
	swapchainInfo.imageExtent = extent;
	swapchainInfo.imageArrayLayers = 1;
	// COLOR_ATTACHMENT for rendering into the image, TRANSFER_SRC so
	// readFramebuffer() can vkCmdCopyImageToBuffer() the swapchain image
	// for screenshots. TRANSFER_SRC is part of every reasonable surface's
	// supportedUsageFlags but we mask against the reported set just to be safe.
	VkImageUsageFlags wantedUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
		wantedUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	} else {
		Log::warn("Swapchain does not support TRANSFER_SRC usage - screenshots will be empty");
	}
	if (caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
		wantedUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	} else {
		Log::warn("Swapchain does not support TRANSFER_DST usage - framebuffer blits to the window will fail");
	}
	swapchainInfo.imageUsage = wantedUsage;
	if (s.graphicsQueueFamily != s.presentQueueFamily) {
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainInfo.queueFamilyIndexCount = 2;
		swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainInfo.queueFamilyIndexCount = 0;
		swapchainInfo.pQueueFamilyIndices = nullptr;
	}
	swapchainInfo.preTransform = caps.currentTransform;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.presentMode = chosenPresentMode;
	swapchainInfo.clipped = VK_TRUE;
	swapchainInfo.oldSwapchain = oldSwapchain;

	result = vkCreateSwapchainKHR(s.device, &swapchainInfo, nullptr, &s.swapchain);
	if (oldSwapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(s.device, oldSwapchain, nullptr);
	}
	if (result != VK_SUCCESS) {
		Log::error("vkCreateSwapchainKHR failed: %s", vkResultStr(result));
		s.swapchain = VK_NULL_HANDLE;
		return false;
	}

	s.swapchainFormat = chosenFormat.format;
	s.swapchainExtent = extent;
	s.swapchainMinImageCount = imageCount;

	// Retrieve swapchain images
	vkGetSwapchainImagesKHR(s.device, s.swapchain, &s.swapchainImageCount, nullptr);
	s.swapchainImages = (VkImage *)core_malloc(sizeof(VkImage) * s.swapchainImageCount);
	vkGetSwapchainImagesKHR(s.device, s.swapchain, &s.swapchainImageCount, s.swapchainImages);

	// Create image views
	s.swapchainImageViews = (VkImageView *)core_malloc(sizeof(VkImageView) * s.swapchainImageCount);
	for (uint32_t i = 0; i < s.swapchainImageCount; ++i) {
		s.swapchainImageViews[i] = VK_NULL_HANDLE;
		VkImageViewCreateInfo viewInfo;
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.pNext = nullptr;
		viewInfo.flags = 0;
		viewInfo.image = s.swapchainImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = s.swapchainFormat;
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		result = vkCreateImageView(s.device, &viewInfo, nullptr, &s.swapchainImageViews[i]);
		if (result != VK_SUCCESS) {
			Log::error("vkCreateImageView [%u] failed: %s", i, vkResultStr(result));
			return false;
		}
	}

	// Allocate glow color attachment images (one per swapchain image) and
	// matching backing memory + image views. The attachment is cleared every
	// frame and discarded on renderpass end, so it is effectively transient;
	// we still need real backing storage because Vulkan 1.0 does not support
	// lazily allocated memory on all implementations.
	s.glowImages = (VkImage *)core_malloc(sizeof(VkImage) * s.swapchainImageCount);
	s.glowImageViews = (VkImageView *)core_malloc(sizeof(VkImageView) * s.swapchainImageCount);
	s.glowMemory = (VkDeviceMemory *)core_malloc(sizeof(VkDeviceMemory) * s.swapchainImageCount);
	for (uint32_t i = 0; i < s.swapchainImageCount; ++i) {
		s.glowImages[i] = VK_NULL_HANDLE;
		s.glowImageViews[i] = VK_NULL_HANDLE;
		s.glowMemory[i] = VK_NULL_HANDLE;
	}
	for (uint32_t i = 0; i < s.swapchainImageCount; ++i) {
		VkImageCreateInfo imgInfo;
		imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imgInfo.pNext = nullptr;
		imgInfo.flags = 0;
		imgInfo.imageType = VK_IMAGE_TYPE_2D;
		imgInfo.format = s.glowFormat;
		imgInfo.extent.width = extent.width;
		imgInfo.extent.height = extent.height;
		imgInfo.extent.depth = 1;
		imgInfo.mipLevels = 1;
		imgInfo.arrayLayers = 1;
		imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imgInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imgInfo.queueFamilyIndexCount = 0;
		imgInfo.pQueueFamilyIndices = nullptr;
		imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		result = vkCreateImage(s.device, &imgInfo, nullptr, &s.glowImages[i]);
		if (result != VK_SUCCESS) {
			Log::error("vkCreateImage (glow[%u]) failed: %s", i, vkResultStr(result));
			return false;
		}

		VkMemoryRequirements req;
		vkGetImageMemoryRequirements(s.device, s.glowImages[i], &req);
		const int32_t memType =
			findMemoryType(s.physicalDevice, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
		if (memType < 0) {
			Log::error("No device-local memory type for glow image");
			return false;
		}
		VkMemoryAllocateInfo memAllocInfo;
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAllocInfo.pNext = nullptr;
		memAllocInfo.allocationSize = req.size;
		memAllocInfo.memoryTypeIndex = (uint32_t)memType;
		result = vkAllocateMemory(s.device, &memAllocInfo, nullptr, &s.glowMemory[i]);
		if (result != VK_SUCCESS) {
			Log::error("vkAllocateMemory (glow[%u]) failed: %s", i, vkResultStr(result));
			return false;
		}
		vkBindImageMemory(s.device, s.glowImages[i], s.glowMemory[i], 0);

		VkImageViewCreateInfo glowViewInfo;
		glowViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		glowViewInfo.pNext = nullptr;
		glowViewInfo.flags = 0;
		glowViewInfo.image = s.glowImages[i];
		glowViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		glowViewInfo.format = s.glowFormat;
		glowViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		glowViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		glowViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		glowViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		glowViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		glowViewInfo.subresourceRange.baseMipLevel = 0;
		glowViewInfo.subresourceRange.levelCount = 1;
		glowViewInfo.subresourceRange.baseArrayLayer = 0;
		glowViewInfo.subresourceRange.layerCount = 1;
		result = vkCreateImageView(s.device, &glowViewInfo, nullptr, &s.glowImageViews[i]);
		if (result != VK_SUCCESS) {
			Log::error("vkCreateImageView (glow[%u]) failed: %s", i, vkResultStr(result));
			return false;
		}
	}

	// Allocate per-frame command buffers from per-frame command pools.
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		s.frameCommandBuffers[i] = VK_NULL_HANDLE;
		if (s.frameCommandPools[i] == VK_NULL_HANDLE) {
			continue;
		}
		VkCommandBufferAllocateInfo frameAllocInfo;
		frameAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		frameAllocInfo.pNext = nullptr;
		frameAllocInfo.commandPool = s.frameCommandPools[i];
		frameAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		frameAllocInfo.commandBufferCount = 1;
		result = vkAllocateCommandBuffers(s.device, &frameAllocInfo, &s.frameCommandBuffers[i]);
		if (result != VK_SUCCESS) {
			Log::error("vkAllocateCommandBuffers (frame %u) failed: %s", i, vkResultStr(result));
			return false;
		}
	}

	// Create per-frame sync objects
	VkSemaphoreCreateInfo semInfo;
	semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semInfo.pNext = nullptr;
	semInfo.flags = 0;

	VkFenceCreateInfo fenceInfo;
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = nullptr;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		result = vkCreateSemaphore(s.device, &semInfo, nullptr, &s.imageAvailableSemaphores[i]);
		if (result != VK_SUCCESS) {
			Log::error("vkCreateSemaphore (imageAvailable[%u]) failed: %s", i, vkResultStr(result));
			return false;
		}
		result = vkCreateFence(s.device, &fenceInfo, nullptr, &s.inFlightFences[i]);
		if (result != VK_SUCCESS) {
			Log::error("vkCreateFence (inFlight[%u]) failed: %s", i, vkResultStr(result));
			return false;
		}
		s.inFlightFencePending[i] = false;
	}

	// One render-finished semaphore per swapchain image, so vkQueuePresentKHR can wait
	// on a semaphore that is uniquely tied to the acquired image (prevents the validation
	// layer warning "is signaled by a different queue submission").
	s.renderFinishedSemaphores = (VkSemaphore *)core_malloc(sizeof(VkSemaphore) * s.swapchainImageCount);
	s.renderFinishedSemaphoreCount = s.swapchainImageCount;
	for (uint32_t i = 0; i < s.swapchainImageCount; ++i) {
		s.renderFinishedSemaphores[i] = VK_NULL_HANDLE;
		result = vkCreateSemaphore(s.device, &semInfo, nullptr, &s.renderFinishedSemaphores[i]);
		if (result != VK_SUCCESS) {
			Log::error("vkCreateSemaphore (renderFinished[%u]) failed: %s", i, vkResultStr(result));
			return false;
		}
	}

	s.currentFrame = 0;
	s.frameAcquired = false;
	Log::info("Swapchain created: %ux%u, %u images, format=%d, presentMode=%u", extent.width, extent.height,
			  s.swapchainImageCount, (int)chosenFormat.format, chosenPresentModeRaw);
	return true;
}

// ===== Rendering Infrastructure =====

// Map a shader DataType + size + normalized flag to a VkFormat used in
// VkVertexInputAttributeDescription. Only the most common combos are covered;
// unsupported combinations fall back to VK_FORMAT_R32G32B32A32_SFLOAT so that
// pipeline creation still succeeds (with a warning) for unusual inputs.
static VkFormat vertexFormatFor(video::DataType type, int size, bool normalized, bool typeIsInt) {
	switch (type) {
	case video::DataType::Float:
		switch (size) {
		case 1: return VK_FORMAT_R32_SFLOAT;
		case 2: return VK_FORMAT_R32G32_SFLOAT;
		case 3: return VK_FORMAT_R32G32B32_SFLOAT;
		case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
		}
		break;
	case video::DataType::UnsignedByte:
		if (normalized) {
			switch (size) {
			case 1: return VK_FORMAT_R8_UNORM;
			case 2: return VK_FORMAT_R8G8_UNORM;
			case 3: return VK_FORMAT_R8G8B8_UNORM;
			case 4: return VK_FORMAT_R8G8B8A8_UNORM;
			}
		} else if (typeIsInt) {
			switch (size) {
			case 1: return VK_FORMAT_R8_UINT;
			case 2: return VK_FORMAT_R8G8_UINT;
			case 3: return VK_FORMAT_R8G8B8_UINT;
			case 4: return VK_FORMAT_R8G8B8A8_UINT;
			}
		} else {
			switch (size) {
			case 1: return VK_FORMAT_R8_USCALED;
			case 2: return VK_FORMAT_R8G8_USCALED;
			case 3: return VK_FORMAT_R8G8B8_USCALED;
			case 4: return VK_FORMAT_R8G8B8A8_USCALED;
			}
		}
		break;
	case video::DataType::Byte:
		if (normalized) {
			switch (size) {
			case 1: return VK_FORMAT_R8_SNORM;
			case 2: return VK_FORMAT_R8G8_SNORM;
			case 3: return VK_FORMAT_R8G8B8_SNORM;
			case 4: return VK_FORMAT_R8G8B8A8_SNORM;
			}
		} else if (typeIsInt) {
			switch (size) {
			case 1: return VK_FORMAT_R8_SINT;
			case 2: return VK_FORMAT_R8G8_SINT;
			case 3: return VK_FORMAT_R8G8B8_SINT;
			case 4: return VK_FORMAT_R8G8B8A8_SINT;
			}
		}
		break;
	case video::DataType::UnsignedShort:
		if (normalized) {
			switch (size) {
			case 1: return VK_FORMAT_R16_UNORM;
			case 2: return VK_FORMAT_R16G16_UNORM;
			case 3: return VK_FORMAT_R16G16B16_UNORM;
			case 4: return VK_FORMAT_R16G16B16A16_UNORM;
			}
		} else if (typeIsInt) {
			switch (size) {
			case 1: return VK_FORMAT_R16_UINT;
			case 2: return VK_FORMAT_R16G16_UINT;
			case 3: return VK_FORMAT_R16G16B16_UINT;
			case 4: return VK_FORMAT_R16G16B16A16_UINT;
			}
		}
		break;
	case video::DataType::Short:
		if (normalized) {
			switch (size) {
			case 1: return VK_FORMAT_R16_SNORM;
			case 2: return VK_FORMAT_R16G16_SNORM;
			case 3: return VK_FORMAT_R16G16B16_SNORM;
			case 4: return VK_FORMAT_R16G16B16A16_SNORM;
			}
		} else if (typeIsInt) {
			switch (size) {
			case 1: return VK_FORMAT_R16_SINT;
			case 2: return VK_FORMAT_R16G16_SINT;
			case 3: return VK_FORMAT_R16G16B16_SINT;
			case 4: return VK_FORMAT_R16G16B16A16_SINT;
			}
		}
		break;
	case video::DataType::UnsignedInt:
		switch (size) {
		case 1: return VK_FORMAT_R32_UINT;
		case 2: return VK_FORMAT_R32G32_UINT;
		case 3: return VK_FORMAT_R32G32B32_UINT;
		case 4: return VK_FORMAT_R32G32B32A32_UINT;
		}
		break;
	case video::DataType::Int:
		switch (size) {
		case 1: return VK_FORMAT_R32_SINT;
		case 2: return VK_FORMAT_R32G32_SINT;
		case 3: return VK_FORMAT_R32G32B32_SINT;
		case 4: return VK_FORMAT_R32G32B32A32_SINT;
		}
		break;
	default:
		break;
	}
	return VK_FORMAT_R32G32B32A32_SFLOAT;
}

static VkPrimitiveTopology primitiveTopologyFor(video::Primitive p) {
	switch (p) {
	case video::Primitive::Points: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case video::Primitive::Lines: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case video::Primitive::LinesAdjacency: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
	case video::Primitive::Triangles: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	case video::Primitive::TrianglesAdjacency: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
	case video::Primitive::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	case video::Primitive::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	default: break;
	}
	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

static VkIndexType indexTypeFor(video::DataType type) {
	switch (type) {
	case video::DataType::UnsignedShort: return VK_INDEX_TYPE_UINT16;
	case video::DataType::UnsignedInt: return VK_INDEX_TYPE_UINT32;
	default: return VK_INDEX_TYPE_UINT32;
	}
}


static bool createRenderPass(VkDevice device, VkFormat colorFormat, VkFormat glowFormat, VkRenderPass &outRenderPass) {
	// Two attachments: the swapchain color (0) and a transient glow target (1)
	// that exists only to satisfy fragment shaders declaring `o_glow` at
	// location 1. Two subpasses:
	//   * subpass 0: main scene rendering, writes both attachments.
	//   * subpass 1: dear-imgui overlay, writes only the swapchain color.
	//     This matches ImGui's Vulkan backend which hardcodes one color-blend
	//     attachment in its pipeline and therefore cannot be used with a
	//     2-attachment subpass.
	VkAttachmentDescription colorAttachment;
	colorAttachment.flags = 0;
	colorAttachment.format = colorFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription glowAttachment;
	glowAttachment.flags = 0;
	glowAttachment.format = glowFormat;
	glowAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	glowAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// Nothing consumes the glow attachment yet, so its contents can be
	// discarded at the end of the render pass.
	glowAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	glowAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	glowAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	glowAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	glowAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorRef;
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference glowRef;
	glowRef.attachment = 1;
	glowRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference mainColorRefs[] = {colorRef, glowRef};

	VkAttachmentDescription attachments[] = {colorAttachment, glowAttachment};

	VkSubpassDescription subpasses[2];
	// Subpass 0: scene (2 color attachments)
	subpasses[0].flags = 0;
	subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[0].inputAttachmentCount = 0;
	subpasses[0].pInputAttachments = nullptr;
	subpasses[0].colorAttachmentCount = lengthof(mainColorRefs);
	subpasses[0].pColorAttachments = mainColorRefs;
	subpasses[0].pResolveAttachments = nullptr;
	subpasses[0].pDepthStencilAttachment = nullptr;
	subpasses[0].preserveAttachmentCount = 0;
	subpasses[0].pPreserveAttachments = nullptr;
	// Subpass 1: ImGui overlay (color only)
	subpasses[1].flags = 0;
	subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[1].inputAttachmentCount = 0;
	subpasses[1].pInputAttachments = nullptr;
	subpasses[1].colorAttachmentCount = 1;
	subpasses[1].pColorAttachments = &colorRef;
	subpasses[1].pResolveAttachments = nullptr;
	subpasses[1].pDepthStencilAttachment = nullptr;
	subpasses[1].preserveAttachmentCount = 0;
	subpasses[1].pPreserveAttachments = nullptr;

	VkSubpassDependency dependencies[2];
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	// Inter-subpass dependency: ensure subpass 0 writes are complete before
	// subpass 1 (ImGui) starts blending on top of the swapchain image.
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = 1;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo;
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.flags = 0;
	renderPassInfo.attachmentCount = lengthof(attachments);
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = lengthof(subpasses);
	renderPassInfo.pSubpasses = subpasses;
	renderPassInfo.dependencyCount = lengthof(dependencies);
	renderPassInfo.pDependencies = dependencies;

	VkResult result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &outRenderPass);
	if (result != VK_SUCCESS) {
		Log::error("vkCreateRenderPass failed: %s", vkResultStr(result));
		return false;
	}
	return true;
}

// Build a VkPipeline for a given program using its reflection data, the
// currently captured vertex input layout and the requested primitive topology.
// The newly created pipeline is returned via outPipeline; the pipeline layout
// and descriptor set layout live on the program and are built lazily on first
// use (they are stable across VAO changes).
static uint64_t hashVertexInput(const VkState::VaoHandle *vao, VkPrimitiveTopology topology);
static uint64_t hashPipelineKey(const VkState::VaoHandle *vao, VkPrimitiveTopology topology, bool hasDepth,
							   uint32_t colorCount);

static bool createUniformRingChunk(VkState &s, VkState::UniformRingChunk &chunk, VkDeviceSize capacity) {
	chunk = VkState::UniformRingChunk{};
	VkBufferCreateInfo bufInfo;
	bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufInfo.pNext = nullptr;
	bufInfo.flags = 0;
	bufInfo.size = capacity;
	bufInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufInfo.queueFamilyIndexCount = 0;
	bufInfo.pQueueFamilyIndices = nullptr;
	if (vkCreateBuffer(s.device, &bufInfo, nullptr, &chunk.buffer) != VK_SUCCESS) {
		Log::error("Failed to create uniform ring buffer");
		chunk.buffer = VK_NULL_HANDLE;
		return false;
	}
	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(s.device, chunk.buffer, &req);
	if (!dedicatedAlloc(s, req.size, req.memoryTypeBits,
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &chunk.memory,
						&chunk.mapped, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ||
		chunk.mapped == nullptr) {
		Log::error("Failed to allocate uniform ring buffer memory");
		vkDestroyBuffer(s.device, chunk.buffer, nullptr);
		chunk.buffer = VK_NULL_HANDLE;
		chunk.memory = VK_NULL_HANDLE;
		return false;
	}
	if (vkBindBufferMemory(s.device, chunk.buffer, chunk.memory, 0) != VK_SUCCESS) {
		Log::error("Failed to bind uniform ring buffer memory");
		if (chunk.mapped != nullptr) {
			vkUnmapMemory(s.device, chunk.memory);
		}
		vkFreeMemory(s.device, chunk.memory, nullptr);
		vkDestroyBuffer(s.device, chunk.buffer, nullptr);
		chunk = VkState::UniformRingChunk{};
		return false;
	}
	chunk.capacity = capacity;
	chunk.offset = 0u;
	return true;
}

static void destroyUniformRingChunk(VkState &s, VkState::UniformRingChunk &chunk) {
	// Route through the deferred buffer path so in-flight command buffers can finish.
	VkState::BufferHandle tmp{};
	tmp.buffer = chunk.buffer;
	tmp.memory = chunk.memory;
	tmp.mapped = chunk.mapped;
	tmp.slabMemory = false;
	tmp.capacity = chunk.capacity;
	destroyBufferAlloc(s, tmp);
	chunk = VkState::UniformRingChunk{};
}

static bool ensureUniformRingChunkSlots(VkState::UniformRingFrame &ring) {
	if (ring.chunkCount < ring.chunkCapacity) {
		return true;
	}
	const uint32_t newCap = ring.chunkCapacity == 0u ? 4u : ring.chunkCapacity * 2u;
	VkState::UniformRingChunk *neu =
		(VkState::UniformRingChunk *)core_malloc(sizeof(VkState::UniformRingChunk) * newCap);
	if (neu == nullptr) {
		return false;
	}
	for (uint32_t i = 0; i < newCap; ++i) {
		neu[i] = VkState::UniformRingChunk{};
	}
	if (ring.chunks != nullptr) {
		for (uint32_t i = 0; i < ring.chunkCount; ++i) {
			neu[i] = ring.chunks[i];
		}
		core_free(ring.chunks);
	}
	ring.chunks = neu;
	ring.chunkCapacity = newCap;
	return true;
}

static bool addUniformRingChunk(VkState &s, VkState::UniformRingFrame &ring, VkDeviceSize minSize) {
	if (ring.chunkCount >= VkState::UNIFORM_RING_MAX_CHUNKS) {
		Log::error("Uniform ring chunk limit reached (%u)", VkState::UNIFORM_RING_MAX_CHUNKS);
		return false;
	}
	if (!ensureUniformRingChunkSlots(ring)) {
		return false;
	}
	VkDeviceSize capacity = VkState::UNIFORM_RING_CHUNK_SIZE;
	if (minSize > capacity) {
		capacity = minSize;
	}
	VkState::UniformRingChunk &chunk = ring.chunks[ring.chunkCount];
	if (!createUniformRingChunk(s, chunk, capacity)) {
		return false;
	}
	++ring.chunkCount;
	return true;
}

static bool createUniformRingBuffer(VkState &s) {
	if (s.uniformRings[0].chunkCount != 0u) {
		return true;
	}
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(s.physicalDevice, &props);
	s.uniformRingAlignment = core_max((VkDeviceSize)256u, props.limits.minUniformBufferOffsetAlignment);
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		if (!addUniformRingChunk(s, s.uniformRings[i], VkState::UNIFORM_RING_CHUNK_SIZE)) {
			return false;
		}
		s.uniformRings[i].currentChunk = 0u;
	}
	return true;
}

static void destroyUniformRingBuffer(VkState &s) {
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		VkState::UniformRingFrame &ring = s.uniformRings[i];
		for (uint32_t c = 0; c < ring.chunkCount; ++c) {
			destroyUniformRingChunk(s, ring.chunks[c]);
		}
		core_free(ring.chunks);
		ring.chunks = nullptr;
		ring.chunkCount = 0u;
		ring.chunkCapacity = 0u;
		ring.currentChunk = 0u;
	}
}

static void resetUniformRingFrame(VkState &s, uint32_t frame) {
	VkState::UniformRingFrame &ring = s.uniformRings[frame];
	ring.currentChunk = 0u;
	for (uint32_t c = 0; c < ring.chunkCount; ++c) {
		ring.chunks[c].offset = 0u;
	}
}

// Copy the current CPU UBO contents into the per-frame ring and return the
// descriptor buffer/offset/range for this draw.
//
// When a buffer has a push-constant overlay (setUniformBufferPushOverlay), and
// only those UBO regions differ from the last ring snapshot, reuse the previous
// ring slot and let the draw path push the overlay fields instead of memcpy'ing
// the whole UBO (GL only uploads the dynamic tail for large vert blocks).
static bool bufferDataMatchesStableExceptPushOverlay(const VkState::BufferHandle &bh) {
	if (bh.stableData == nullptr || bh.stableDataSize != bh.size || bh.data == nullptr) {
		return false;
	}
	if (bh.pushOverlayFieldCount == 0u) {
		return core_memcmp(bh.data, bh.stableData, bh.size) == 0;
	}
	size_t pos = 0u;
	for (uint32_t i = 0; i < bh.pushOverlayFieldCount; ++i) {
		const UniformPushOverlayField &f = bh.pushOverlayFields[i];
		if ((size_t)f.uboOffset > bh.size || (size_t)f.uboOffset + (size_t)f.size > bh.size) {
			return false;
		}
		if (f.uboOffset < pos) {
			return false; // fields must be sorted by uboOffset
		}
		if (f.uboOffset > pos) {
			if (core_memcmp(bh.data + pos, bh.stableData + pos, (size_t)f.uboOffset - pos) != 0) {
				return false;
			}
		}
		pos = (size_t)f.uboOffset + (size_t)f.size;
	}
	if (pos < bh.size) {
		if (core_memcmp(bh.data + pos, bh.stableData + pos, bh.size - pos) != 0) {
			return false;
		}
	}
	return true;
}

static bool storeStableUniformSnapshot(VkState::BufferHandle &bh) {
	if (bh.data == nullptr || bh.size == 0u) {
		return false;
	}
	if (bh.stableData == nullptr || bh.stableDataSize != bh.size) {
		if (bh.stableData != nullptr) {
			core_free(bh.stableData);
			bh.stableData = nullptr;
			bh.stableDataSize = 0u;
		}
		bh.stableData = (uint8_t *)core_malloc(bh.size);
		if (bh.stableData == nullptr) {
			return false;
		}
		bh.stableDataSize = bh.size;
	}
	core_memcpy(bh.stableData, bh.data, bh.size);
	return true;
}

static bool snapshotUniformToRing(VkState &s, VkState::BufferHandle &bh, VkBuffer &outBuffer,
								  VkDeviceSize &outOffset, VkDeviceSize &outRange) {
	if (bh.data == nullptr || bh.size == 0u) {
		return false;
	}

	// Unchanged CPU UBO (no bufferSubData/bufferData since last snapshot): reuse.
	if (!bh.partialDirty && bh.stableRingFrame == s.currentFrame && bh.stableRingBuffer != VK_NULL_HANDLE) {
		outBuffer = bh.stableRingBuffer;
		outOffset = bh.stableRingOffset;
		outRange = (VkDeviceSize)bh.size;
		return true;
	}

	// Only push-overlay fields changed vs last ring snapshot: reuse slot.
	if (bh.partialDirty && bh.pushOverlayFieldCount > 0u && bh.stableRingFrame == s.currentFrame &&
		bh.stableRingBuffer != VK_NULL_HANDLE && bufferDataMatchesStableExceptPushOverlay(bh)) {
		outBuffer = bh.stableRingBuffer;
		outOffset = bh.stableRingOffset;
		outRange = (VkDeviceSize)bh.size;
		bh.partialDirty = false;
		return true;
	}

	VkState::UniformRingFrame &ring = s.uniformRings[s.currentFrame];
	if (ring.chunkCount == 0u) {
		if (!addUniformRingChunk(s, ring, core_max(VkState::UNIFORM_RING_CHUNK_SIZE, (VkDeviceSize)bh.size))) {
			return false;
		}
	}
	const VkDeviceSize align = s.uniformRingAlignment == 0u ? 256u : s.uniformRingAlignment;
	const VkDeviceSize needed = (VkDeviceSize)bh.size;
	for (;;) {
		VkState::UniformRingChunk &chunk = ring.chunks[ring.currentChunk];
		if (chunk.mapped != nullptr) {
			VkDeviceSize offset = (chunk.offset + align - 1u) / align * align;
			if (offset + needed <= chunk.capacity) {
				core_memcpy((uint8_t *)chunk.mapped + offset, bh.data, (size_t)needed);
				chunk.offset = offset + needed;
				outBuffer = chunk.buffer;
				outOffset = offset;
				outRange = needed;
				bh.stableRingFrame = s.currentFrame;
				bh.stableRingBuffer = outBuffer;
				bh.stableRingOffset = outOffset;
				(void)storeStableUniformSnapshot(bh);
				bh.partialDirty = false;
				return true;
			}
		}
		if (ring.currentChunk + 1u < ring.chunkCount) {
			++ring.currentChunk;
			continue;
		}
		const uint32_t prevCount = ring.chunkCount;
		if (!addUniformRingChunk(s, ring, core_max(VkState::UNIFORM_RING_CHUNK_SIZE, needed))) {
			Log::error("Uniform ring buffer grow failed (need %u bytes)", (unsigned)needed);
			return false;
		}
		ring.currentChunk = prevCount;
		Log::debug("Uniform ring grew to %u chunks for frame %u", ring.chunkCount, s.currentFrame);
	}
}

static bool ensurePipelineCacheCapacity(VkState::ProgramHandle &prog) {
	if (prog.pipelineCount < prog.pipelineCapacity) {
		return true;
	}
	const uint32_t newCap = prog.pipelineCapacity == 0u ? 16u : prog.pipelineCapacity * 2u;
	VkState::ProgramHandle::PipelineEntry *newArr = new VkState::ProgramHandle::PipelineEntry[newCap]();
	if (prog.pipelines != nullptr) {
		for (uint32_t i = 0; i < prog.pipelineCount; ++i) {
			newArr[i] = prog.pipelines[i];
		}
		delete[] prog.pipelines;
	}
	prog.pipelines = newArr;
	prog.pipelineCapacity = newCap;
	return true;
}

static bool createGraphicsPipelineForProgram(VkState &s, VkState::ProgramHandle &prog, VkState::VaoHandle *vao,
											 VkPrimitiveTopology topology, VkRenderPass renderPass,
											 uint32_t colorAttachmentCount, bool hasDepth, VkPipeline &outPipeline) {
	outPipeline = VK_NULL_HANDLE;
	if (prog.vertShader == InvalidId || prog.fragShader == InvalidId) {
		return false;
	}
	VkState::ShaderHandle *vertH = findShaderHandle(s, prog.vertShader);
	VkState::ShaderHandle *fragH = findShaderHandle(s, prog.fragShader);
	if (vertH == nullptr || fragH == nullptr) {
		return false;
	}
	if (vertH->module == VK_NULL_HANDLE || fragH->module == VK_NULL_HANDLE) {
		return false;
	}

	// Pipeline layout is deferred to prepareDrawState() so that
	// registerShaderBindings() (called after init()) has populated the binding table.

	VkPipelineShaderStageCreateInfo stages[2];
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].pNext = nullptr;
	stages[0].flags = 0;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = vertH->module;
	stages[0].pName = "main";
	stages[0].pSpecializationInfo = nullptr;
	stages[1] = stages[0];
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = fragH->module;

	// ---- Vertex input state from captured VAO ----
	VkVertexInputBindingDescription bindings[16];
	VkVertexInputAttributeDescription attrs[16];
	uint32_t bindingCount = 0u;
	uint32_t attrCount = 0u;
	if (vao != nullptr) {
		// One VkVertexInputBindingDescription per distinct bufferIndex in the
		// captured attributes; stride is taken from the attribute's own stride
		// (all attributes sharing a bufferIndex must have the same stride).
		int seenBufferIndex[16];
		for (int i = 0; i < 16; ++i) {
			seenBufferIndex[i] = -1;
		}
		for (uint32_t i = 0; i < vao->attributeCount && attrCount < 16u; ++i) {
			const Attribute &a = vao->attributes[i];
			// Shadowmap vertex shaders only consume position (location 0). Passing the
			// full voxel VAO (info/light attributes) trips unused-location warnings.
			if (SDL_strstr(prog.name.c_str(), "shadowmap") != nullptr && a.location != 0) {
				continue;
			}
			int bIdx = -1;
			for (uint32_t b = 0; b < bindingCount; ++b) {
				if (seenBufferIndex[b] == a.bufferIndex) {
					bIdx = (int)b;
					break;
				}
			}
			if (bIdx < 0 && bindingCount < 16u) {
				bIdx = (int)bindingCount;
				seenBufferIndex[bindingCount] = a.bufferIndex;
				bindings[bindingCount].binding = bindingCount;
				bindings[bindingCount].stride = (uint32_t)a.stride;
				bindings[bindingCount].inputRate =
					a.divisor == 0 ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
				++bindingCount;
			}
			if (bIdx < 0) {
				continue;
			}
			attrs[attrCount].binding = (uint32_t)bIdx;
			attrs[attrCount].location = (uint32_t)a.location;
			attrs[attrCount].format = vertexFormatFor(a.type, a.size, a.normalized, a.typeIsInt);
			attrs[attrCount].offset = (uint32_t)a.offset;
			++attrCount;
		}
	}

	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.pNext = nullptr;
	vertexInputInfo.flags = 0;
	vertexInputInfo.vertexBindingDescriptionCount = bindingCount;
	vertexInputInfo.pVertexBindingDescriptions = bindingCount == 0u ? nullptr : bindings;
	vertexInputInfo.vertexAttributeDescriptionCount = attrCount;
	vertexInputInfo.pVertexAttributeDescriptions = attrCount == 0u ? nullptr : attrs;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.pNext = nullptr;
	inputAssembly.flags = 0;
	inputAssembly.topology = topology;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamicState;
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pNext = nullptr;
	dynamicState.flags = 0;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynStates;

	VkPipelineViewportStateCreateInfo viewportState;
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;
	viewportState.flags = 0;
	viewportState.viewportCount = 1;
	viewportState.pViewports = nullptr;
	viewportState.scissorCount = 1;
	viewportState.pScissors = nullptr;

	VkPipelineRasterizationStateCreateInfo rasterizer;
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.pNext = nullptr;
	rasterizer.flags = 0;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	{
		const RendererState &rs = rendererState();
		if (rs.pendingStates[core::enumVal(State::CullFace)]) {
			rasterizer.cullMode = cullModeFor(rs.pendingCullFace);
		} else {
			rasterizer.cullMode = VK_CULL_MODE_NONE;
		}
		// Negative-height viewport maps GL Y-up clip space onto Vulkan's
		// top-left framebuffer. Voxel meshes are CCW in that space (same as
		// GL_CCW), so keep COUNTER_CLOCKWISE; CLOCKWISE culls outer faces.
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	}
	{
		const RendererState &rs = rendererState();
		const bool depthBias = rs.pendingStates[core::enumVal(State::PolygonOffsetFill)] ||
							   rs.pendingStates[core::enumVal(State::PolygonOffsetLine)] ||
							   rs.pendingStates[core::enumVal(State::PolygonOffsetPoint)];
		rasterizer.depthBiasEnable = depthBias ? VK_TRUE : VK_FALSE;
		rasterizer.depthBiasConstantFactor = depthBias ? rs.pendingPolygonOffset.y : 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = depthBias ? rs.pendingPolygonOffset.x : 0.0f;
	}
	rasterizer.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampling;
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.pNext = nullptr;
	multisampling.flags = 0;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.minSampleShading = 0.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	{
		const RendererState &rs = rendererState();
		const bool blend = rs.pendingStates[core::enumVal(State::Blend)];
		colorBlendAttachment.blendEnable = blend ? VK_TRUE : VK_FALSE;
		if (blend) {
			colorBlendAttachment.srcColorBlendFactor = blendFactorFor(rs.pendingBlendSrcRGB);
			colorBlendAttachment.dstColorBlendFactor = blendFactorFor(rs.pendingBlendDestRGB);
			colorBlendAttachment.colorBlendOp = blendOpFor(rs.pendingBlendEquation);
			colorBlendAttachment.srcAlphaBlendFactor = blendFactorFor(rs.pendingBlendSrcAlpha);
			colorBlendAttachment.dstAlphaBlendFactor = blendFactorFor(rs.pendingBlendDestAlpha);
			colorBlendAttachment.alphaBlendOp = blendOpFor(rs.pendingBlendEquation);
		} else {
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		}
	}
	{
		const RendererState &rsMask = rendererState();
		VkColorComponentFlags writeMask = 0;
		if (rsMask.pendingColorMask[0]) {
			writeMask |= VK_COLOR_COMPONENT_R_BIT;
		}
		if (rsMask.pendingColorMask[1]) {
			writeMask |= VK_COLOR_COMPONENT_G_BIT;
		}
		if (rsMask.pendingColorMask[2]) {
			writeMask |= VK_COLOR_COMPONENT_B_BIT;
		}
		if (rsMask.pendingColorMask[3]) {
			writeMask |= VK_COLOR_COMPONENT_A_BIT;
		}
		colorBlendAttachment.colorWriteMask = writeMask;
	}
	// Match the active subpass's color attachment count: main pass = 2 (scene
	// + glow), off-screen FBO render passes = whatever the FBO declared.
	const uint32_t blendCount = colorAttachmentCount;
	VkPipelineColorBlendAttachmentState colorBlendAttachments[8];
	for (uint32_t i = 0; i < blendCount && i < lengthof(colorBlendAttachments); ++i) {
		colorBlendAttachments[i] = colorBlendAttachment;
		if ((s.drawBufferMask & (uint8_t)(1u << i)) == 0u) {
			colorBlendAttachments[i].blendEnable = VK_FALSE;
			colorBlendAttachments[i].colorWriteMask = 0;
		}
	}

	VkPipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.pNext = nullptr;
	colorBlending.flags = 0;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = blendCount;
	colorBlending.pAttachments = blendCount > 0u ? colorBlendAttachments : nullptr;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineDepthStencilStateCreateInfo depthStencil;
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.pNext = nullptr;
	depthStencil.flags = 0;
	{
		const RendererState &rs = rendererState();
		const bool depthTest = hasDepth && rs.pendingStates[core::enumVal(State::DepthTest)];
		const bool depthWrite = hasDepth && rs.pendingStates[core::enumVal(State::DepthMask)];
		depthStencil.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
		depthStencil.depthWriteEnable = depthWrite ? VK_TRUE : VK_FALSE;
		depthStencil.depthCompareOp = compareOpFor(rs.pendingDepthFunc);
	}
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {};
	depthStencil.back = {};
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;

	VkGraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = stages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = hasDepth ? &depthStencil : nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = prog.layout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	VkResult result =
		vkCreateGraphicsPipelines(s.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &outPipeline);
	if (result != VK_SUCCESS) {
		Log::error("vkCreateGraphicsPipelines failed for %s: %s", prog.name.c_str(), vkResultStr(result));
		outPipeline = VK_NULL_HANDLE;
		return false;
	}
	Log::debug("Built pipeline for %s (bindings=%u, attrs=%u, colorAtt=%u, depth=%d)", prog.name.c_str(),
			   bindingCount, attrCount, colorAttachmentCount, hasDepth ? 1 : 0);
	return true;
}

static VkDescriptorType descriptorTypeForBinding(const ShaderResourceBinding &rb) {
	if (rb.type == ShaderResourceBinding::UniformBuffer) {
		// Dynamic offsets let us reuse one descriptor set across draws that only
		// change uniform contents (snapshotted into the per-frame ring).
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	}
	if (rb.type == ShaderResourceBinding::StorageImage) {
		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	}
	return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
}

// Destroy cached pipelines + layout so the next draw rebuilds them. Keeps the
// descriptor set layout unless @p destroyDescriptorSetLayout is true.
static void invalidateProgramPipelines(VkState &s, VkState::ProgramHandle &prog, bool destroyDescriptorSetLayout) {
	if (s.device == VK_NULL_HANDLE) {
		prog.layout = VK_NULL_HANDLE;
		if (destroyDescriptorSetLayout) {
			prog.descriptorSetLayout = VK_NULL_HANDLE;
		}
		prog.pipelineCount = 0u;
		prog.computePipeline = VK_NULL_HANDLE;
		return;
	}
	for (uint32_t j = 0; j < prog.pipelineCount; ++j) {
		if (prog.pipelines != nullptr && prog.pipelines[j].pipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(s.device, prog.pipelines[j].pipeline, nullptr);
			prog.pipelines[j].pipeline = VK_NULL_HANDLE;
		}
	}
	prog.pipelineCount = 0u;
	if (prog.computePipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(s.device, prog.computePipeline, nullptr);
		prog.computePipeline = VK_NULL_HANDLE;
	}
	if (prog.layout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(s.device, prog.layout, nullptr);
		prog.layout = VK_NULL_HANDLE;
	}
	if (destroyDescriptorSetLayout && prog.descriptorSetLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(s.device, prog.descriptorSetLayout, nullptr);
		prog.descriptorSetLayout = VK_NULL_HANDLE;
	}
}

static bool ensureProgramLayout(VkState &s, VkState::ProgramHandle &prog) {
	if (prog.layout != VK_NULL_HANDLE) {
		return true;
	}
	VkDescriptorSetLayoutBinding dslBindings[16];
	uint32_t dslBindingCount = 0u;
	for (uint32_t i = 0; i < prog.registeredBindingCount; ++i) {
		const ShaderResourceBinding &rb = prog.registeredBindings[i];
		VkDescriptorSetLayoutBinding &b = dslBindings[dslBindingCount++];
		b.binding = rb.binding;
		b.descriptorType = descriptorTypeForBinding(rb);
		b.descriptorCount = 1u;
		b.stageFlags = 0;
		if (rb.stageFlags & 1) {
			b.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
		}
		if (rb.stageFlags & 2) {
			b.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		if (rb.stageFlags & 4) {
			b.stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;
		}
		if (rb.stageFlags & 8) {
			b.stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
		}
		b.pImmutableSamplers = nullptr;
	}
	if (prog.computeShader != InvalidId && dslBindingCount == 0u &&
		prog.registeredBindingCount < VkState::ProgramHandle::MAX_BINDINGS) {
		VkDescriptorSetLayoutBinding &b = dslBindings[dslBindingCount++];
		b.binding = 0;
		b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		b.descriptorCount = 1u;
		b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		b.pImmutableSamplers = nullptr;
		ShaderResourceBinding &rb = prog.registeredBindings[prog.registeredBindingCount++];
		rb.binding = 0;
		rb.type = ShaderResourceBinding::StorageImage;
		rb.stageFlags = 8;
	}
	if (prog.descriptorSetLayout == VK_NULL_HANDLE) {
		VkDescriptorSetLayoutCreateInfo dslInfo;
		dslInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		dslInfo.pNext = nullptr;
		dslInfo.flags = 0;
		dslInfo.bindingCount = dslBindingCount;
		dslInfo.pBindings = dslBindingCount == 0u ? nullptr : dslBindings;
		if (vkCreateDescriptorSetLayout(s.device, &dslInfo, nullptr, &prog.descriptorSetLayout) != VK_SUCCESS) {
			Log::error("vkCreateDescriptorSetLayout failed for %s", prog.name.c_str());
			prog.descriptorSetLayout = VK_NULL_HANDLE;
			return false;
		}
	}

	VkPipelineLayoutCreateInfo plInfo;
	plInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	plInfo.pNext = nullptr;
	plInfo.flags = 0;
	plInfo.setLayoutCount = (prog.descriptorSetLayout != VK_NULL_HANDLE) ? 1u : 0u;
	plInfo.pSetLayouts = (prog.descriptorSetLayout != VK_NULL_HANDLE) ? &prog.descriptorSetLayout : nullptr;
	VkPushConstantRange pushRange;
	pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushRange.offset = 0;
	pushRange.size = prog.pushConstantSize;
	plInfo.pushConstantRangeCount = prog.pushConstantSize > 0u ? 1u : 0u;
	plInfo.pPushConstantRanges = prog.pushConstantSize > 0u ? &pushRange : nullptr;
	if (vkCreatePipelineLayout(s.device, &plInfo, nullptr, &prog.layout) != VK_SUCCESS) {
		Log::error("vkCreatePipelineLayout failed for %s", prog.name.c_str());
		prog.layout = VK_NULL_HANDLE;
		return false;
	}
	return true;
}

static bool ensureComputePipeline(VkState &s, VkState::ProgramHandle &prog) {
	if (prog.computePipeline != VK_NULL_HANDLE) {
		return true;
	}
	if (prog.computeShader == InvalidId) {
		return false;
	}
	VkState::ShaderHandle *compH = findShaderHandle(s, prog.computeShader);
	if (compH == nullptr || compH->module == VK_NULL_HANDLE) {
		Log::error("Missing compute shader module for %s", prog.name.c_str());
		return false;
	}
	if (!ensureProgramLayout(s, prog)) {
		Log::error("Failed to create compute pipeline layout for %s", prog.name.c_str());
		return false;
	}
	VkPipelineShaderStageCreateInfo stage;
	core_memset(&stage, 0, sizeof(stage));
	stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stage.module = compH->module;
	stage.pName = "main";
	VkComputePipelineCreateInfo ci;
	core_memset(&ci, 0, sizeof(ci));
	ci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	ci.stage = stage;
	ci.layout = prog.layout;
	ci.basePipelineIndex = -1;
	const VkResult r = vkCreateComputePipelines(s.device, VK_NULL_HANDLE, 1, &ci, nullptr, &prog.computePipeline);
	if (r != VK_SUCCESS) {
		Log::error("vkCreateComputePipelines failed for %s: %s", prog.name.c_str(), vkResultStr(r));
		prog.computePipeline = VK_NULL_HANDLE;
		return false;
	}
	return true;
}

// Find a cached pipeline for this program that matches the current vertex
// input signature + topology, creating a new one on miss. Returns VK_NULL_HANDLE
// on failure.
static VkPipeline findOrCreatePipeline(VkState &s, VkState::ProgramHandle &prog, VkState::VaoHandle *vao,
									   VkPrimitiveTopology topology) {
	if (!ensureProgramLayout(s, prog)) {
		return VK_NULL_HANDLE;
	}

	VkRenderPass rp = s.currentRenderPass != VK_NULL_HANDLE ? s.currentRenderPass : s.renderPass;
	uint32_t colorCount = s.currentColorAttachmentCount;
	if (rp == s.renderPass || rp == s.mainResumeRenderPass) {
		colorCount = 2u;
	}
	const bool hasDepth = s.currentRenderPassHasDepth;
	const uint64_t wantHash = hashPipelineKey(vao, topology, hasDepth, colorCount);
	for (uint32_t i = 0; i < prog.pipelineCount; ++i) {
		if (prog.pipelines[i].hash == wantHash && prog.pipelines[i].renderPass == rp &&
			prog.pipelines[i].hasDepth == hasDepth) {
			return prog.pipelines[i].pipeline;
		}
	}
	if (!ensurePipelineCacheCapacity(prog)) {
		Log::error("Failed to grow pipeline cache for %s", prog.name.c_str());
		return VK_NULL_HANDLE;
	}
	VkPipeline pipe = VK_NULL_HANDLE;
	if (!createGraphicsPipelineForProgram(s, prog, vao, topology, rp, colorCount, hasDepth, pipe)) {
		return VK_NULL_HANDLE;
	}
	prog.pipelines[prog.pipelineCount].hash = wantHash;
	prog.pipelines[prog.pipelineCount].renderPass = rp;
	prog.pipelines[prog.pipelineCount].hasDepth = hasDepth;
	prog.pipelines[prog.pipelineCount].pipeline = pipe;
	++prog.pipelineCount;
	return pipe;
}

static bool createFramebuffers(VkDevice device, VkRenderPass renderPass, const VkImageView *imageViews,
							   const VkImageView *glowImageViews, uint32_t imageCount, VkExtent2D extent,
							   VkFramebuffer *&outFramebuffers) {
	outFramebuffers = (VkFramebuffer *)core_malloc(sizeof(VkFramebuffer) * imageCount);
	if (outFramebuffers == nullptr) {
		return false;
	}

	for (uint32_t i = 0; i < imageCount; ++i) {
		VkImageView attachments[] = {imageViews[i], glowImageViews[i]};

		VkFramebufferCreateInfo framebufferInfo;
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.pNext = nullptr;
		framebufferInfo.flags = 0;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = lengthof(attachments);
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &outFramebuffers[i]);
		if (result != VK_SUCCESS) {
			Log::error("vkCreateFramebuffer [%u] failed: %s", i, vkResultStr(result));
			for (uint32_t j = 0; j < i; ++j) {
				vkDestroyFramebuffer(device, outFramebuffers[j], nullptr);
			}
			core_free(outFramebuffers);
			outFramebuffers = nullptr;
			return false;
		}
	}

	return true;
}

} // namespace _priv

static inline _priv::VkState &vkstate() {
	static _priv::VkState s;
	return s;
}

RendererState &rendererState() {
	return (RendererState &)vkstate();
}

void setup() {
}

RendererContext createContext(SDL_Window *window) {
	_priv::VkState &s = vkstate();
	s.window = window;
	s.validationEnabled = false;

	if (flextVkInit() == -1) {
		Log::error("Could not initialize Vulkan loader: %s", SDL_GetError());
		return nullptr;
	}

	unsigned int extensionCount = 0;
#if SDL_VERSION_ATLEAST(3, 2, 0)
	const char *const *sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
	if (sdlExtensions == nullptr && extensionCount == 0) {
		Log::error("SDL_Vulkan_GetInstanceExtensions failed: %s", SDL_GetError());
		flextVkShutdown();
		return nullptr;
	}
#else
	if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr)) {
		Log::error("SDL_Vulkan_GetInstanceExtensions (count) failed: %s", SDL_GetError());
		flextVkShutdown();
		return nullptr;
	}
	const char **sdlExtensions = (const char **)core_malloc(sizeof(const char *) * extensionCount);
	if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, sdlExtensions)) {
		Log::error("SDL_Vulkan_GetInstanceExtensions failed: %s", SDL_GetError());
		core_free((void *)sdlExtensions);
		flextVkShutdown();
		return nullptr;
	}
#endif

	const uint32_t maxExtraExtensions = 1;
	const char **extensions = (const char **)core_malloc(sizeof(const char *) * (extensionCount + maxExtraExtensions));
	for (uint32_t i = 0; i < extensionCount; ++i) {
		extensions[i] = sdlExtensions[i];
	}

	uint32_t enabledExtensionCount = extensionCount;
	const char *enabledLayers[1] = {"VK_LAYER_KHRONOS_validation"};
	uint32_t enabledLayerCount = 0;
	const bool haveDebugReport = _priv::hasInstanceExtension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	// Validation (especially sync) can drop a busy scene from ~100 FPS to single digits.
	// Default: off. Opt in with VENGI_VK_VALIDATION=core|sync|full (or 0 to force off).
	// In DEBUG builds, "core" is the default when the env var is unset.
	enum class ValidationMode { Off, Core, Sync, Full };
	ValidationMode validationMode = ValidationMode::Off;
#ifdef DEBUG
	validationMode = ValidationMode::Core;
#endif
	if (const char *env = SDL_getenv("VENGI_VK_VALIDATION")) {
		if (env[0] == '\0' || !core_strcasecmp(env, "0") || !core_strcasecmp(env, "off") ||
			!core_strcasecmp(env, "none")) {
			validationMode = ValidationMode::Off;
		} else if (!core_strcasecmp(env, "core") || !core_strcasecmp(env, "1")) {
			validationMode = ValidationMode::Core;
		} else if (!core_strcasecmp(env, "sync")) {
			validationMode = ValidationMode::Sync;
		} else if (!core_strcasecmp(env, "full") || !core_strcasecmp(env, "all")) {
			validationMode = ValidationMode::Full;
		} else {
			Log::warn("Unknown VENGI_VK_VALIDATION=%s (use off|core|sync|full)", env);
		}
	}

	const bool wantValidation = validationMode != ValidationMode::Off;
	const bool wantSync = validationMode == ValidationMode::Sync || validationMode == ValidationMode::Full;
	const bool wantBestPractices = validationMode == ValidationMode::Full;
	if (wantValidation && _priv::hasInstanceLayer(enabledLayers[0])) {
		enabledLayerCount = 1;
		s.validationEnabled = true;
	} else if (wantValidation) {
		Log::warn("Vulkan validation layer not available: %s", enabledLayers[0]);
	}
	if (haveDebugReport) {
		extensions[enabledExtensionCount++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
	} else if (s.validationEnabled) {
		Log::warn("Vulkan debug report extension not available: %s", VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	for (uint32_t i = 0; i < enabledExtensionCount; ++i) {
		Log::debug("Vulkan instance extension [%u]: %s", i, extensions[i]);
	}

	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = "vengi";
	appInfo.applicationVersion = 1;
	appInfo.pEngineName = "vengi";
	appInfo.engineVersion = 1;
	appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 1, 0);

	// GPU-AV stays off: extremely slow and pulls in unused device features.
	VkValidationFeatureEnableEXT validationEnables[2];
	uint32_t validationEnableCount = 0u;
	if (wantBestPractices) {
		validationEnables[validationEnableCount++] = VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT;
	}
	if (wantSync) {
		validationEnables[validationEnableCount++] = VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
	}
	VkValidationFeaturesEXT validationFeatures;
	validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
	validationFeatures.pNext = nullptr;
	validationFeatures.enabledValidationFeatureCount = validationEnableCount;
	validationFeatures.pEnabledValidationFeatures = validationEnableCount > 0u ? validationEnables : nullptr;
	validationFeatures.disabledValidationFeatureCount = 0;
	validationFeatures.pDisabledValidationFeatures = nullptr;

	const VkBool32 settingTrue = VK_TRUE;
	const VkBool32 settingFalse = VK_FALSE;
	const VkBool32 settingSync = wantSync ? VK_TRUE : VK_FALSE;
	const VkBool32 settingBest = wantBestPractices ? VK_TRUE : VK_FALSE;
	const VkLayerSettingEXT layerSettings[] = {
		{"VK_LAYER_KHRONOS_validation", "validate_core", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &settingTrue},
		{"VK_LAYER_KHRONOS_validation", "validate_sync", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &settingSync},
		{"VK_LAYER_KHRONOS_validation", "gpuav_enable", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &settingFalse},
		{"VK_LAYER_KHRONOS_validation", "validate_best_practices", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1,
		 &settingBest}};
	VkLayerSettingsCreateInfoEXT layerSettingsInfo;
	layerSettingsInfo.sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT;
	layerSettingsInfo.pNext = nullptr;
	layerSettingsInfo.settingCount = lengthof(layerSettings);
	layerSettingsInfo.pSettings = layerSettings;

	VkDebugReportCallbackCreateInfoEXT debugInfo;
	debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	debugInfo.pNext = nullptr;
	// Errors + warnings only. Info/debug/perf spam is a major FPS tax with many draws.
	debugInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	debugInfo.pfnCallback = _priv::vkDebugReportCallback;
	debugInfo.pUserData = &s;

	VkInstanceCreateInfo createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledLayerCount = enabledLayerCount;
	createInfo.ppEnabledLayerNames = enabledLayerCount > 0 ? enabledLayers : nullptr;
	createInfo.enabledExtensionCount = enabledExtensionCount;
	createInfo.ppEnabledExtensionNames = extensions;

	if (s.validationEnabled) {
		layerSettingsInfo.pNext = validationEnableCount > 0u ? &validationFeatures : nullptr;
		if (haveDebugReport) {
			debugInfo.pNext = &layerSettingsInfo;
			createInfo.pNext = &debugInfo;
		} else {
			createInfo.pNext = &layerSettingsInfo;
		}
		if (wantBestPractices) {
			Log::info("Vulkan validation enabled (core, sync, best practices) via VENGI_VK_VALIDATION");
		} else if (wantSync) {
			Log::info("Vulkan validation enabled (core, sync) via VENGI_VK_VALIDATION");
		} else {
			Log::info("Vulkan validation enabled (core only)");
		}
	} else if (haveDebugReport) {
		createInfo.pNext = &debugInfo;
	}

	VkResult result = vkCreateInstance(&createInfo, nullptr, &s.instance);
	if (result != VK_SUCCESS && s.validationEnabled) {
		Log::warn("vkCreateInstance with validation features failed (%s), retrying with layer only",
				  _priv::vkResultStr(result));
		createInfo.pNext = haveDebugReport ? (const void *)&debugInfo : nullptr;
		debugInfo.pNext = nullptr;
		result = vkCreateInstance(&createInfo, nullptr, &s.instance);
	}
	if (result != VK_SUCCESS && s.validationEnabled) {
		Log::warn("vkCreateInstance with validation layer failed (%s), retrying without validation",
				  _priv::vkResultStr(result));
		createInfo.pNext = nullptr;
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
		s.validationEnabled = false;
		result = vkCreateInstance(&createInfo, nullptr, &s.instance);
	}
	core_free((void *)extensions);
#if !SDL_VERSION_ATLEAST(3, 2, 0)
	core_free((void *)sdlExtensions);
#endif
	if (result != VK_SUCCESS) {
		Log::error("vkCreateInstance failed: %s", _priv::vkResultStr(result));
		flextVkShutdown();
		return nullptr;
	}

	flextVkInitInstance(s.instance);
	if (haveDebugReport && vkCreateDebugReportCallbackEXT != nullptr) {
		debugInfo.pNext = nullptr;
		result = vkCreateDebugReportCallbackEXT(s.instance, &debugInfo, nullptr, &s.debugReportCallback);
		if (result != VK_SUCCESS) {
			Log::warn("vkCreateDebugReportCallbackEXT failed: %s", _priv::vkResultStr(result));
			s.debugReportCallback = VK_NULL_HANDLE;
		}
	}

#if SDL_VERSION_ATLEAST(3, 2, 0)
	if (!SDL_Vulkan_CreateSurface(window, s.instance, nullptr, &s.surface)) {
#else
	if (!SDL_Vulkan_CreateSurface(window, s.instance, &s.surface)) {
#endif
		Log::error("SDL_Vulkan_CreateSurface failed: %s", SDL_GetError());
		vkDestroyInstance(s.instance, nullptr);
		s.instance = VK_NULL_HANDLE;
		flextVkShutdown();
		return nullptr;
	}

	return (RendererContext)&s;
}

void destroyContext(RendererContext &context) {
	_priv::VkState &s = vkstate();

	if (s.device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(s.device);
		for (uint32_t i = 0; i < _priv::MAX_FRAMES_IN_FLIGHT; ++i) {
			if (s.frameCommandPools[i] != VK_NULL_HANDLE) {
				vkResetCommandPool(s.device, s.frameCommandPools[i], 0);
			}
		}
		s.frameAcquired = false;
		s.mainRenderPassActive = false;
		s.fboRenderPassActive = false;
		for (uint32_t i = 0; i < _priv::MAX_FRAMES_IN_FLIGHT; ++i) {
			if (s.pendingFreeSetCount[i] == 0u || s.descriptorPool == VK_NULL_HANDLE) {
				continue;
			}
			vkFreeDescriptorSets(s.device, s.descriptorPool, s.pendingFreeSetCount[i], s.pendingFreeSets[i]);
			s.pendingFreeSetCount[i] = 0u;
		}
	}

	_priv::destroySwapchainResources(s, true);
	_priv::destroyUniformRingBuffer(s);
	// Flush buffers queued by destroyUniformRingBuffer / handle teardown.
	if (s.device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(s.device);
		for (uint32_t i = 0; i < s.pendingBufferDestroyCount; ++i) {
			const _priv::VkState::PendingBufferDestroy &p = s.pendingBufferDestroys[i];
			if (p.mapped != nullptr && p.memory != VK_NULL_HANDLE && p.freeMemory) {
				vkUnmapMemory(s.device, p.memory);
			}
			if (p.buffer != VK_NULL_HANDLE) {
				vkDestroyBuffer(s.device, p.buffer, nullptr);
			}
			if (p.freeMemory && p.memory != VK_NULL_HANDLE) {
				vkFreeMemory(s.device, p.memory, nullptr);
			}
		}
		s.pendingBufferDestroyCount = 0u;
	}

	if (s.device != VK_NULL_HANDLE) {
		if (s.shaderHandles != nullptr) {
			for (uint32_t i = 0; i < s.shaderHandleCount; ++i) {
				if (s.shaderHandles[i].module != VK_NULL_HANDLE) {
					vkDestroyShaderModule(s.device, s.shaderHandles[i].module, nullptr);
					s.shaderHandles[i].module = VK_NULL_HANDLE;
				}
			}
		}
		if (s.programHandles != nullptr) {
			for (uint32_t i = 0; i < s.programHandleCount; ++i) {
				_priv::VkState::ProgramHandle &p = s.programHandles[i];
				for (uint32_t j = 0; j < p.pipelineCount; ++j) {
					if (p.pipelines != nullptr && p.pipelines[j].pipeline != VK_NULL_HANDLE) {
						vkDestroyPipeline(s.device, p.pipelines[j].pipeline, nullptr);
						p.pipelines[j].pipeline = VK_NULL_HANDLE;
					}
				}
				if (p.computePipeline != VK_NULL_HANDLE) {
					vkDestroyPipeline(s.device, p.computePipeline, nullptr);
					p.computePipeline = VK_NULL_HANDLE;
				}
				if (p.layout != VK_NULL_HANDLE) {
					vkDestroyPipelineLayout(s.device, p.layout, nullptr);
					p.layout = VK_NULL_HANDLE;
				}
				if (p.descriptorSetLayout != VK_NULL_HANDLE) {
					vkDestroyDescriptorSetLayout(s.device, p.descriptorSetLayout, nullptr);
					p.descriptorSetLayout = VK_NULL_HANDLE;
				}
			}
		}
		if (s.bufferHandles != nullptr) {
			for (uint32_t i = 0; i < s.bufferHandleCount; ++i) {
				_priv::destroyBufferAlloc(s, s.bufferHandles[i]);
			}
		}
		if (s.textureHandles != nullptr) {
			for (uint32_t i = 0; i < s.textureHandleCount; ++i) {
				_priv::VkState::TextureHandle &h = s.textureHandles[i];
				if (!h.alive) {
					continue;
				}
				// Free imgui descriptor sets before samplers/images they reference.
				if (h.imguiSet != VK_NULL_HANDLE && s.descriptorPool != VK_NULL_HANDLE) {
					vkFreeDescriptorSets(s.device, s.descriptorPool, 1, &h.imguiSet);
					h.imguiSet = VK_NULL_HANDLE;
				}
				if (h.sampler != VK_NULL_HANDLE) {
					vkDestroySampler(s.device, h.sampler, nullptr);
				}
				if (h.view != VK_NULL_HANDLE) {
					vkDestroyImageView(s.device, h.view, nullptr);
				}
				if (h.image != VK_NULL_HANDLE) {
					vkDestroyImage(s.device, h.image, nullptr);
				}
				if (h.memory != VK_NULL_HANDLE && !h.slabMemory) {
					vkFreeMemory(s.device, h.memory, nullptr);
				}
				h.sampler = VK_NULL_HANDLE;
				h.view = VK_NULL_HANDLE;
				h.image = VK_NULL_HANDLE;
				h.memory = VK_NULL_HANDLE;
				h.slabMemory = false;
				h.alive = false;
			}
		}
		if (s.placeholderSampler != VK_NULL_HANDLE) {
			vkDestroySampler(s.device, s.placeholderSampler, nullptr);
			s.placeholderSampler = VK_NULL_HANDLE;
		}
		if (s.placeholderImageView != VK_NULL_HANDLE) {
			vkDestroyImageView(s.device, s.placeholderImageView, nullptr);
			s.placeholderImageView = VK_NULL_HANDLE;
		}
		if (s.placeholderImage != VK_NULL_HANDLE) {
			vkDestroyImage(s.device, s.placeholderImage, nullptr);
			s.placeholderImage = VK_NULL_HANDLE;
		}
		s.placeholderImageMemory = VK_NULL_HANDLE;
		if (s.placeholderDepthArraySampler != VK_NULL_HANDLE) {
			vkDestroySampler(s.device, s.placeholderDepthArraySampler, nullptr);
			s.placeholderDepthArraySampler = VK_NULL_HANDLE;
		}
		if (s.placeholderDepthArrayView != VK_NULL_HANDLE) {
			vkDestroyImageView(s.device, s.placeholderDepthArrayView, nullptr);
			s.placeholderDepthArrayView = VK_NULL_HANDLE;
		}
		if (s.placeholderDepthArrayImage != VK_NULL_HANDLE) {
			vkDestroyImage(s.device, s.placeholderDepthArrayImage, nullptr);
			s.placeholderDepthArrayImage = VK_NULL_HANDLE;
		}
		s.placeholderDepthArrayMemory = VK_NULL_HANDLE;
		for (uint32_t i = 0; i < s.pendingFboDestroyCount; ++i) {
			const _priv::VkState::PendingFboDestroy &p = s.pendingFboDestroys[i];
			if (p.framebuffer != VK_NULL_HANDLE) {
				vkDestroyFramebuffer(s.device, p.framebuffer, nullptr);
			}
			if (p.renderPass != VK_NULL_HANDLE) {
				vkDestroyRenderPass(s.device, p.renderPass, nullptr);
			}
			if (p.renderPassLoad != VK_NULL_HANDLE) {
				vkDestroyRenderPass(s.device, p.renderPassLoad, nullptr);
			}
			if (p.renderPassClearDefined != VK_NULL_HANDLE) {
				vkDestroyRenderPass(s.device, p.renderPassClearDefined, nullptr);
			}
			if (p.renderPassColorClear != VK_NULL_HANDLE) {
				vkDestroyRenderPass(s.device, p.renderPassColorClear, nullptr);
			}
			if (p.renderPassColorClearDefined != VK_NULL_HANDLE) {
				vkDestroyRenderPass(s.device, p.renderPassColorClearDefined, nullptr);
			}
			if (p.extraView != VK_NULL_HANDLE) {
				vkDestroyImageView(s.device, p.extraView, nullptr);
			}
		}
		s.pendingFboDestroyCount = 0u;
		for (uint32_t i = 0; i < s.pendingBufferDestroyCount; ++i) {
			const _priv::VkState::PendingBufferDestroy &p = s.pendingBufferDestroys[i];
			if (p.mapped != nullptr && p.memory != VK_NULL_HANDLE && p.freeMemory) {
				vkUnmapMemory(s.device, p.memory);
			}
			if (p.buffer != VK_NULL_HANDLE) {
				vkDestroyBuffer(s.device, p.buffer, nullptr);
			}
			if (p.freeMemory && p.memory != VK_NULL_HANDLE) {
				vkFreeMemory(s.device, p.memory, nullptr);
			}
		}
		s.pendingBufferDestroyCount = 0u;
		if (s.framebufferHandles != nullptr) {
			for (uint32_t i = 0; i < s.framebufferHandleCount; ++i) {
				_priv::VkState::FramebufferHandle &h = s.framebufferHandles[i];
				if (!h.alive) {
					continue;
				}
				if (h.framebuffer != VK_NULL_HANDLE) {
					vkDestroyFramebuffer(s.device, h.framebuffer, nullptr);
				}
				if (h.renderPass != VK_NULL_HANDLE) {
					vkDestroyRenderPass(s.device, h.renderPass, nullptr);
				}
				if (h.renderPassLoad != VK_NULL_HANDLE) {
					vkDestroyRenderPass(s.device, h.renderPassLoad, nullptr);
				}
				if (h.renderPassClearDefined != VK_NULL_HANDLE) {
					vkDestroyRenderPass(s.device, h.renderPassClearDefined, nullptr);
				}
				if (h.renderPassColorClear != VK_NULL_HANDLE) {
					vkDestroyRenderPass(s.device, h.renderPassColorClear, nullptr);
				}
				if (h.renderPassColorClearDefined != VK_NULL_HANDLE) {
					vkDestroyRenderPass(s.device, h.renderPassColorClearDefined, nullptr);
				}
				if (h.depthLayerView != VK_NULL_HANDLE) {
					vkDestroyImageView(s.device, h.depthLayerView, nullptr);
				}
				h.framebuffer = VK_NULL_HANDLE;
				h.renderPass = VK_NULL_HANDLE;
				h.renderPassLoad = VK_NULL_HANDLE;
				h.renderPassClearDefined = VK_NULL_HANDLE;
				h.renderPassColorClear = VK_NULL_HANDLE;
				h.renderPassColorClearDefined = VK_NULL_HANDLE;
				h.depthLayerView = VK_NULL_HANDLE;
				h.alive = false;
			}
		}
	}
	if (s.pendingFboDestroys != nullptr) {
		core_free(s.pendingFboDestroys);
		s.pendingFboDestroys = nullptr;
	}
	s.pendingFboDestroyCapacity = 0u;
	if (s.pendingBufferDestroys != nullptr) {
		core_free(s.pendingBufferDestroys);
		s.pendingBufferDestroys = nullptr;
	}
	s.pendingBufferDestroyCapacity = 0u;
	for (uint32_t f = 0; f < _priv::MAX_FRAMES_IN_FLIGHT; ++f) {
		if (s.pendingFreeSets[f] != nullptr) {
			core_free(s.pendingFreeSets[f]);
			s.pendingFreeSets[f] = nullptr;
		}
		s.pendingFreeSetCount[f] = 0u;
		s.pendingFreeSetCapacity[f] = 0u;
	}
	if (s.descriptorPool != VK_NULL_HANDLE && s.device != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(s.device, s.descriptorPool, nullptr);
		s.descriptorPool = VK_NULL_HANDLE;
	}
	if (s.renderPass != VK_NULL_HANDLE && s.device != VK_NULL_HANDLE) {
		vkDestroyRenderPass(s.device, s.renderPass, nullptr);
		s.renderPass = VK_NULL_HANDLE;
	}
	if (s.mainResumeRenderPass != VK_NULL_HANDLE && s.device != VK_NULL_HANDLE) {
		vkDestroyRenderPass(s.device, s.mainResumeRenderPass, nullptr);
		s.mainResumeRenderPass = VK_NULL_HANDLE;
	}

	if (s.commandBuffer != VK_NULL_HANDLE && s.commandPool != VK_NULL_HANDLE && s.device != VK_NULL_HANDLE) {
		vkFreeCommandBuffers(s.device, s.commandPool, 1, &s.commandBuffer);
		s.commandBuffer = VK_NULL_HANDLE;
	}
	if (s.commandPool != VK_NULL_HANDLE && s.device != VK_NULL_HANDLE) {
		vkDestroyCommandPool(s.device, s.commandPool, nullptr);
		s.commandPool = VK_NULL_HANDLE;
	}
	for (uint32_t i = 0; i < _priv::MAX_FRAMES_IN_FLIGHT; ++i) {
		if (s.frameCommandPools[i] != VK_NULL_HANDLE && s.device != VK_NULL_HANDLE) {
			vkDestroyCommandPool(s.device, s.frameCommandPools[i], nullptr);
			s.frameCommandPools[i] = VK_NULL_HANDLE;
		}
	}
	_priv::destroyMemorySlabs(s);
	if (s.device != VK_NULL_HANDLE) {
		vkDestroyDevice(s.device, nullptr);
		s.device = VK_NULL_HANDLE;
	}
	if (s.surface != VK_NULL_HANDLE && s.instance != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(s.instance, s.surface, nullptr);
		s.surface = VK_NULL_HANDLE;
	}
	if (s.debugReportCallback != VK_NULL_HANDLE && s.instance != VK_NULL_HANDLE &&
		vkDestroyDebugReportCallbackEXT != nullptr) {
		vkDestroyDebugReportCallbackEXT(s.instance, s.debugReportCallback, nullptr);
		s.debugReportCallback = VK_NULL_HANDLE;
	}
	if (s.instance != VK_NULL_HANDLE) {
		vkDestroyInstance(s.instance, nullptr);
		s.instance = VK_NULL_HANDLE;
	}
	if (s.shaderHandles != nullptr) {
		delete[] s.shaderHandles;
		s.shaderHandles = nullptr;
	}
	s.shaderHandleCount = 0;
	s.shaderHandleCapacity = 0;
	if (s.programHandles != nullptr) {
		for (uint32_t i = 0; i < s.programHandleCount; ++i) {
			delete[] s.programHandles[i].pipelines;
			s.programHandles[i].pipelines = nullptr;
		}
		delete[] s.programHandles;
		s.programHandles = nullptr;
	}
	s.programHandleCount = 0;
	s.programHandleCapacity = 0;
	if (s.bufferHandles != nullptr) {
		for (uint32_t i = 0; i < s.bufferHandleCount; ++i) {
			if (s.bufferHandles[i].data != nullptr) {
				core_free((void *)s.bufferHandles[i].data);
				s.bufferHandles[i].data = nullptr;
			}
			if (s.bufferHandles[i].stableData != nullptr) {
				core_free(s.bufferHandles[i].stableData);
				s.bufferHandles[i].stableData = nullptr;
				s.bufferHandles[i].stableDataSize = 0u;
			}
		}
		delete[] s.bufferHandles;
		s.bufferHandles = nullptr;
	}
	s.bufferHandleCount = 0;
	s.bufferHandleCapacity = 0;
	if (s.bufferIdIndex != nullptr) {
		core_free(s.bufferIdIndex);
		s.bufferIdIndex = nullptr;
	}
	s.bufferIdIndexCapacity = 0u;
	if (s.vaoHandles != nullptr) {
		delete[] s.vaoHandles;
		s.vaoHandles = nullptr;
	}
	s.vaoHandleCount = 0;
	s.vaoHandleCapacity = 0;
	if (s.vaoIdIndex != nullptr) {
		core_free(s.vaoIdIndex);
		s.vaoIdIndex = nullptr;
	}
	s.vaoIdIndexCapacity = 0u;
	if (s.textureHandles != nullptr) {
		delete[] s.textureHandles;
		s.textureHandles = nullptr;
	}
	s.textureHandleCount = 0;
	s.textureHandleCapacity = 0;
	if (s.framebufferHandles != nullptr) {
		delete[] s.framebufferHandles;
		s.framebufferHandles = nullptr;
	}
	s.framebufferHandleCount = 0;
	s.framebufferHandleCapacity = 0;
	s.nextHandleId = 1u;
	s.validationEnabled = false;
	flextVkShutdown();
	context = nullptr;
}

bool init(int windowWidth, int windowHeight, float scaleFactor) {
	_priv::VkState &s = vkstate();
	s.windowWidth = windowWidth;
	s.windowHeight = windowHeight;
	s.scaleFactor = scaleFactor;
	// Vulkan NDC is upper-left origin with depth in [0, 1].
	s.clipOriginLowerLeft = false;
	s.clipDepthZeroToOne = true;

	if (s.instance == VK_NULL_HANDLE) {
		Log::error("Vulkan instance not created - call createContext first");
		return false;
	}

	uint32_t physicalDeviceCount = 0;
	VkResult result = vkEnumeratePhysicalDevices(s.instance, &physicalDeviceCount, nullptr);
	if (result != VK_SUCCESS || physicalDeviceCount == 0) {
		Log::error("vkEnumeratePhysicalDevices failed or no devices: %s", _priv::vkResultStr(result));
		return false;
	}

	VkPhysicalDevice *physicalDevices = (VkPhysicalDevice *)core_malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);
	result = vkEnumeratePhysicalDevices(s.instance, &physicalDeviceCount, physicalDevices);
	if (result != VK_SUCCESS) {
		Log::error("vkEnumeratePhysicalDevices (fill) failed: %s", _priv::vkResultStr(result));
		core_free((void *)physicalDevices);
		return false;
	}

	int bestScore = -1;
	s.physicalDevice = VK_NULL_HANDLE;
	s.graphicsQueueFamily = UINT32_MAX;
	s.presentQueueFamily = UINT32_MAX;

	for (uint32_t i = 0; i < physicalDeviceCount; ++i) {
		VkPhysicalDevice candidate = physicalDevices[i];

		uint32_t extCount = 0;
		vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extCount, nullptr);
		VkExtensionProperties *exts = (VkExtensionProperties *)core_malloc(sizeof(VkExtensionProperties) * extCount);
		vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extCount, exts);
		bool hasSwapchain = false;
		for (uint32_t e = 0; e < extCount; ++e) {
			if (SDL_strcmp(exts[e].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
				hasSwapchain = true;
				break;
			}
		}
		core_free(exts);
		if (!hasSwapchain) {
			Log::debug("Skipping device %u: missing VK_KHR_swapchain", i);
			continue;
		}

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueFamilyCount, nullptr);
		VkQueueFamilyProperties *queueFamilies =
			(VkQueueFamilyProperties *)core_malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueFamilyCount, queueFamilies);

		uint32_t graphicsFamily = UINT32_MAX;
		uint32_t presentFamily = UINT32_MAX;
		for (uint32_t q = 0; q < queueFamilyCount; ++q) {
			if ((queueFamilies[q].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) ==
					(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT) &&
				graphicsFamily == UINT32_MAX) {
				graphicsFamily = q;
			}
			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(candidate, q, s.surface, &presentSupport);
			if (presentSupport && presentFamily == UINT32_MAX) {
				presentFamily = q;
			}
		}
		core_free(queueFamilies);

		if (graphicsFamily == UINT32_MAX || presentFamily == UINT32_MAX) {
			Log::debug("Skipping device %u: no suitable queue families", i);
			continue;
		}

		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(candidate, &props);
		VkPhysicalDeviceFeatures unusedFeatures;
		vkGetPhysicalDeviceFeatures(candidate, &unusedFeatures);

		int score = 0;
		switch (props.deviceType) {
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			score = 4;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			score = 3;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			score = 2;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			score = 1;
			break;
		default:
			score = 0;
			break;
		}

		Log::info("Vulkan physical device [%u]: %s (type=%d, score=%d)", i, props.deviceName, (int)props.deviceType,
				  score);

		if (score > bestScore) {
			bestScore = score;
			s.physicalDevice = candidate;
			s.graphicsQueueFamily = graphicsFamily;
			s.presentQueueFamily = presentFamily;
		}
	}
	core_free((void *)physicalDevices);

	if (s.physicalDevice == VK_NULL_HANDLE) {
		Log::error("No suitable Vulkan physical device found");
		return false;
	}

	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(s.physicalDevice, &props);
		Log::info("Selected Vulkan device: %s (graphics family=%u, present family=%u)", props.deviceName,
				  s.graphicsQueueFamily, s.presentQueueFamily);
	}

	const float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfos[2];
	uint32_t queueCreateCount = 1;

	queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[0].pNext = nullptr;
	queueCreateInfos[0].flags = 0;
	queueCreateInfos[0].queueFamilyIndex = s.graphicsQueueFamily;
	queueCreateInfos[0].queueCount = 1;
	queueCreateInfos[0].pQueuePriorities = &queuePriority;

	if (s.presentQueueFamily != s.graphicsQueueFamily) {
		queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[1].pNext = nullptr;
		queueCreateInfos[1].flags = 0;
		queueCreateInfos[1].queueFamilyIndex = s.presentQueueFamily;
		queueCreateInfos[1].queueCount = 1;
		queueCreateInfos[1].pQueuePriorities = &queuePriority;
		queueCreateCount = 2;
	}

	const char *deviceExtensions[1];
	uint32_t deviceExtensionCount = 1;
	deviceExtensions[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

	VkPhysicalDeviceFeatures availableFeatures{};
	vkGetPhysicalDeviceFeatures(s.physicalDevice, &availableFeatures);

	VkPhysicalDeviceFeatures enabledFeatures = {};
	enabledFeatures.independentBlend = availableFeatures.independentBlend;
	enabledFeatures.samplerAnisotropy = availableFeatures.samplerAnisotropy;
	enabledFeatures.fillModeNonSolid = availableFeatures.fillModeNonSolid;
	enabledFeatures.wideLines = availableFeatures.wideLines;
	enabledFeatures.geometryShader = availableFeatures.geometryShader;
	enabledFeatures.multiDrawIndirect = availableFeatures.multiDrawIndirect;
	enabledFeatures.imageCubeArray = availableFeatures.imageCubeArray;
	enabledFeatures.sampleRateShading = availableFeatures.sampleRateShading;
	enabledFeatures.fragmentStoresAndAtomics = availableFeatures.fragmentStoresAndAtomics;
	enabledFeatures.vertexPipelineStoresAndAtomics = availableFeatures.vertexPipelineStoresAndAtomics;
	enabledFeatures.shaderInt64 = availableFeatures.shaderInt64;
	enabledFeatures.shaderClipDistance = availableFeatures.shaderClipDistance;
	enabledFeatures.shaderCullDistance = availableFeatures.shaderCullDistance;
	enabledFeatures.shaderStorageImageExtendedFormats = availableFeatures.shaderStorageImageExtendedFormats;
	enabledFeatures.shaderStorageImageWriteWithoutFormat = availableFeatures.shaderStorageImageWriteWithoutFormat;
	enabledFeatures.shaderStorageImageReadWithoutFormat = availableFeatures.shaderStorageImageReadWithoutFormat;

	VkDeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = nullptr;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.queueCreateInfoCount = queueCreateCount;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = nullptr;
	deviceCreateInfo.enabledExtensionCount = deviceExtensionCount;
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
	deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

	result = vkCreateDevice(s.physicalDevice, &deviceCreateInfo, nullptr, &s.device);
	if (result != VK_SUCCESS) {
		Log::error("vkCreateDevice failed: %s", _priv::vkResultStr(result));
		return false;
	}

	vkGetDeviceQueue(s.device, s.graphicsQueueFamily, 0, &s.graphicsQueue);
	vkGetDeviceQueue(s.device, s.presentQueueFamily, 0, &s.presentQueue);

	VkCommandPoolCreateInfo commandPoolCreateInfo;
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext = nullptr;
	commandPoolCreateInfo.flags = 0;
	commandPoolCreateInfo.queueFamilyIndex = s.graphicsQueueFamily;

	result = vkCreateCommandPool(s.device, &commandPoolCreateInfo, nullptr, &s.commandPool);
	if (result != VK_SUCCESS) {
		Log::error("vkCreateCommandPool failed: %s", _priv::vkResultStr(result));
		return false;
	}
	for (uint32_t i = 0; i < _priv::MAX_FRAMES_IN_FLIGHT; ++i) {
		result = vkCreateCommandPool(s.device, &commandPoolCreateInfo, nullptr, &s.frameCommandPools[i]);
		if (result != VK_SUCCESS) {
			Log::error("vkCreateCommandPool (frame %u) failed: %s", i, _priv::vkResultStr(result));
			return false;
		}
	}

	// Utility command buffer (single, for one-off operations)
	VkCommandBufferAllocateInfo allocateInfo;
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.pNext = nullptr;
	allocateInfo.commandPool = s.commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;

	result = vkAllocateCommandBuffers(s.device, &allocateInfo, &s.commandBuffer);
	if (result != VK_SUCCESS) {
		Log::error("vkAllocateCommandBuffers failed: %s", _priv::vkResultStr(result));
		return false;
	}

	if (!_priv::createSwapchain(s)) {
		return false;
	}

	// Create render pass and framebuffers
	if (s.renderPass == VK_NULL_HANDLE) {
		if (!_priv::createRenderPass(s.device, s.swapchainFormat, s.glowFormat, s.renderPass)) {
			Log::error("Failed to create render pass");
			return false;
		}
	}
	if (s.framebuffers == nullptr) {
		if (!_priv::createFramebuffers(s.device, s.renderPass, s.swapchainImageViews, s.glowImageViews,
									   s.swapchainImageCount, s.swapchainExtent, s.framebuffers)) {
			Log::error("Failed to create framebuffers");
			return false;
		}
	}

	// Descriptor pool: ImGui + per-draw sets. UBO count is high because each
	// draw snapshots uniforms into a fresh set while older sets stay live until
	// their frame slot is reused (MAX_FRAMES_IN_FLIGHT).
	if (s.descriptorPool == VK_NULL_HANDLE) {
		VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
											{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4000},
											{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
											{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
											{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
											{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
											{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4000},
											{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
											{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 16000},
											{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
											{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
		VkDescriptorPoolCreateInfo poolInfo;
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.pNext = nullptr;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 16000;
		poolInfo.poolSizeCount = (uint32_t)lengthof(poolSizes);
		poolInfo.pPoolSizes = poolSizes;
		VkResult poolRes = vkCreateDescriptorPool(s.device, &poolInfo, nullptr, &s.descriptorPool);
		if (poolRes != VK_SUCCESS) {
			Log::error("vkCreateDescriptorPool failed: %s", _priv::vkResultStr(poolRes));
			return false;
		}
	}

	if (!_priv::createUniformRingBuffer(s)) {
		return false;
	}

	// Match GLRenderer defaults so pending blend *::Max sentinels are never used.
	blendEquation(BlendEquation::Add);
	blendFuncSeparate(BlendMode::SourceAlpha, BlendMode::OneMinusSourceAlpha, BlendMode::One,
					  BlendMode::OneMinusSourceAlpha);
	enable(State::DepthMask);

	// Create the 1x1 placeholder image used by every combined-image-sampler
	// descriptor binding until the engine's texture upload path is wired
	// through. This guarantees vkCmdDraw never sees an unbound sampler slot.
	if (s.placeholderImage == VK_NULL_HANDLE) {
		VkImageCreateInfo ici;
		ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ici.pNext = nullptr;
		ici.flags = 0;
		ici.imageType = VK_IMAGE_TYPE_2D;
		ici.format = VK_FORMAT_R8G8B8A8_UNORM;
		ici.extent.width = 1;
		ici.extent.height = 1;
		ici.extent.depth = 1;
		ici.mipLevels = 1;
		ici.arrayLayers = 1;
		ici.samples = VK_SAMPLE_COUNT_1_BIT;
		ici.tiling = VK_IMAGE_TILING_OPTIMAL;
		ici.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ici.queueFamilyIndexCount = 0;
		ici.pQueueFamilyIndices = nullptr;
		ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		if (vkCreateImage(s.device, &ici, nullptr, &s.placeholderImage) != VK_SUCCESS) {
			Log::error("Failed to create placeholder image");
			return false;
		}
		VkMemoryRequirements mr;
		vkGetImageMemoryRequirements(s.device, s.placeholderImage, &mr);
		VkDeviceSize phOffset = 0;
		if (!_priv::slabAlloc(s, mr.size, mr.alignment, mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
							  &s.placeholderImageMemory, &phOffset, nullptr)) {
			Log::error("Failed to allocate placeholder image memory");
			return false;
		}
		vkBindImageMemory(s.device, s.placeholderImage, s.placeholderImageMemory, phOffset);

		// Transition layout to SHADER_READ_ONLY_OPTIMAL via a one-shot command
		// buffer. The image content is left undefined; samplers will return
		// undefined data which is fine for the placeholder use case.
		VkCommandBufferAllocateInfo cbai;
		cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cbai.pNext = nullptr;
		cbai.commandPool = s.commandPool;
		cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cbai.commandBufferCount = 1;
		VkCommandBuffer ccb = VK_NULL_HANDLE;
		if (vkAllocateCommandBuffers(s.device, &cbai, &ccb) == VK_SUCCESS && ccb != VK_NULL_HANDLE) {
			VkCommandBufferBeginInfo cbbi;
			cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cbbi.pNext = nullptr;
			cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			cbbi.pInheritanceInfo = nullptr;
			vkBeginCommandBuffer(ccb, &cbbi);
			VkImageMemoryBarrier bar;
			bar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			bar.pNext = nullptr;
			bar.srcAccessMask = 0;
			bar.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			bar.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			bar.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			bar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bar.image = s.placeholderImage;
			bar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bar.subresourceRange.baseMipLevel = 0;
			bar.subresourceRange.levelCount = 1;
			bar.subresourceRange.baseArrayLayer = 0;
			bar.subresourceRange.layerCount = 1;
			vkCmdPipelineBarrier(ccb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
								 0, nullptr, 1, &bar);
			VkClearColorValue clearColor{};
			vkCmdClearColorImage(ccb, s.placeholderImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1,
								 &bar.subresourceRange);
			bar.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			bar.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			bar.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			bar.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			vkCmdPipelineBarrier(ccb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
								 nullptr, 0, nullptr, 1, &bar);
			vkEndCommandBuffer(ccb);
			VkSubmitInfo si;
			si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			si.pNext = nullptr;
			si.waitSemaphoreCount = 0;
			si.pWaitSemaphores = nullptr;
			si.pWaitDstStageMask = nullptr;
			si.commandBufferCount = 1;
			si.pCommandBuffers = &ccb;
			si.signalSemaphoreCount = 0;
			si.pSignalSemaphores = nullptr;
			vkQueueSubmit(s.graphicsQueue, 1, &si, VK_NULL_HANDLE);
			vkQueueWaitIdle(s.graphicsQueue);
			vkFreeCommandBuffers(s.device, s.commandPool, 1, &ccb);
		}

		VkImageViewCreateInfo ivci;
		ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ivci.pNext = nullptr;
		ivci.flags = 0;
		ivci.image = s.placeholderImage;
		ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ivci.format = VK_FORMAT_R8G8B8A8_UNORM;
		ivci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ivci.subresourceRange.baseMipLevel = 0;
		ivci.subresourceRange.levelCount = 1;
		ivci.subresourceRange.baseArrayLayer = 0;
		ivci.subresourceRange.layerCount = 1;
		if (vkCreateImageView(s.device, &ivci, nullptr, &s.placeholderImageView) != VK_SUCCESS) {
			Log::error("Failed to create placeholder image view");
			return false;
		}
		VkSamplerCreateInfo sci;
		sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sci.pNext = nullptr;
		sci.flags = 0;
		sci.magFilter = VK_FILTER_NEAREST;
		sci.minFilter = VK_FILTER_NEAREST;
		sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sci.mipLodBias = 0.0f;
		sci.anisotropyEnable = VK_FALSE;
		sci.maxAnisotropy = 1.0f;
		sci.compareEnable = VK_FALSE;
		sci.compareOp = VK_COMPARE_OP_NEVER;
		sci.minLod = 0.0f;
		sci.maxLod = 0.0f;
		sci.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		sci.unnormalizedCoordinates = VK_FALSE;
		if (vkCreateSampler(s.device, &sci, nullptr, &s.placeholderSampler) != VK_SUCCESS) {
			Log::error("Failed to create placeholder sampler");
			return false;
		}

		// Depth 2D array + compare sampler for sampler2DArrayShadow (e.g. u_shadowmaptex).
		const uint32_t placeholderLayers = 4u;
		VkImageCreateInfo dici{};
		dici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		dici.imageType = VK_IMAGE_TYPE_2D;
		dici.format = VK_FORMAT_D32_SFLOAT;
		dici.extent.width = 4u;
		dici.extent.height = 4u;
		dici.extent.depth = 1;
		dici.mipLevels = 1;
		dici.arrayLayers = placeholderLayers;
		dici.samples = VK_SAMPLE_COUNT_1_BIT;
		dici.tiling = VK_IMAGE_TILING_OPTIMAL;
		dici.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
					 VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		dici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		dici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		if (vkCreateImage(s.device, &dici, nullptr, &s.placeholderDepthArrayImage) != VK_SUCCESS) {
			Log::error("Failed to create placeholder depth array image");
			return false;
		}
		VkMemoryRequirements dmr;
		vkGetImageMemoryRequirements(s.device, s.placeholderDepthArrayImage, &dmr);
		VkDeviceSize dOffset = 0;
		if (!_priv::slabAlloc(s, dmr.size, dmr.alignment, dmr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
							  &s.placeholderDepthArrayMemory, &dOffset, nullptr)) {
			Log::error("Failed to allocate placeholder depth array memory");
			return false;
		}
		vkBindImageMemory(s.device, s.placeholderDepthArrayImage, s.placeholderDepthArrayMemory, dOffset);

		VkCommandBufferAllocateInfo pcbai{};
		pcbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		pcbai.commandPool = s.commandPool;
		pcbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		pcbai.commandBufferCount = 1;
		VkCommandBuffer pccb = VK_NULL_HANDLE;
		if (vkAllocateCommandBuffers(s.device, &pcbai, &pccb) == VK_SUCCESS && pccb != VK_NULL_HANDLE) {
			VkCommandBufferBeginInfo pcbbi{};
			pcbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			pcbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkBeginCommandBuffer(pccb, &pcbbi);
			VkImageMemoryBarrier pbar{};
			pbar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			pbar.srcAccessMask = 0;
			pbar.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			pbar.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			pbar.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			pbar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			pbar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			pbar.image = s.placeholderDepthArrayImage;
			pbar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			pbar.subresourceRange.levelCount = 1;
			pbar.subresourceRange.layerCount = placeholderLayers;
			vkCmdPipelineBarrier(pccb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
								 0, nullptr, 1, &pbar);
			VkClearDepthStencilValue clearDepth{};
			clearDepth.depth = 1.0f;
			vkCmdClearDepthStencilImage(pccb, s.placeholderDepthArrayImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
										&clearDepth, 1, &pbar.subresourceRange);
			pbar.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			pbar.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			pbar.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			pbar.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			vkCmdPipelineBarrier(pccb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
								 nullptr, 0, nullptr, 1, &pbar);
			vkEndCommandBuffer(pccb);
			VkSubmitInfo psi{};
			psi.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			psi.commandBufferCount = 1;
			psi.pCommandBuffers = &pccb;
			vkQueueSubmit(s.graphicsQueue, 1, &psi, VK_NULL_HANDLE);
			vkQueueWaitIdle(s.graphicsQueue);
			vkFreeCommandBuffers(s.device, s.commandPool, 1, &pccb);
		}

		VkImageViewCreateInfo divci{};
		divci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		divci.image = s.placeholderDepthArrayImage;
		divci.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		divci.format = VK_FORMAT_D32_SFLOAT;
		divci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		divci.subresourceRange.levelCount = 1;
		divci.subresourceRange.layerCount = placeholderLayers;
		if (vkCreateImageView(s.device, &divci, nullptr, &s.placeholderDepthArrayView) != VK_SUCCESS) {
			Log::error("Failed to create placeholder depth array image view");
			return false;
		}

		VkSamplerCreateInfo dsci{};
		dsci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		dsci.magFilter = VK_FILTER_LINEAR;
		dsci.minFilter = VK_FILTER_LINEAR;
		dsci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		dsci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		dsci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		dsci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		dsci.anisotropyEnable = VK_FALSE;
		dsci.compareEnable = VK_TRUE;
		dsci.compareOp = VK_COMPARE_OP_LESS;
		dsci.minLod = 0.0f;
		dsci.maxLod = 0.0f;
		dsci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		if (vkCreateSampler(s.device, &dsci, nullptr, &s.placeholderDepthArraySampler) != VK_SUCCESS) {
			Log::error("Failed to create placeholder depth array sampler");
			return false;
		}
	}

	// Set renderer feature flags. In Vulkan 1.0, instanced arrays, compute shaders, SSBOs,
	// texture float/half-float are all mandatory. Query physical device for optional features.
	{
		VkPhysicalDeviceFeatures devFeatures;
		vkGetPhysicalDeviceFeatures(s.physicalDevice, &devFeatures);

		// Vulkan 1.0 mandatory features
		renderState().features[core::enumVal(Feature::TextureFloat)] = true;
		renderState().features[core::enumVal(Feature::TextureHalfFloat)] = true;
		renderState().features[core::enumVal(Feature::InstancedArrays)] = true;
		renderState().features[core::enumVal(Feature::ComputeShaders)] = true;
		renderState().features[core::enumVal(Feature::ShaderStorageBufferObject)] = true;

		// DirectStateAccess does not apply to Vulkan (no global mutable binding state)
		renderState().features[core::enumVal(Feature::DirectStateAccess)] = false;
		// TransformFeedback requires VK_EXT_transform_feedback
		renderState().features[core::enumVal(Feature::TransformFeedback)] = false;

		// Multi-draw indirect: query physical device
		renderState().features[core::enumVal(Feature::MultiDrawIndirect)] = (devFeatures.multiDrawIndirect == VK_TRUE);

		// DXT (BCn) texture compression: check format support
		VkFormatProperties fmtProps;
		vkGetPhysicalDeviceFormatProperties(s.physicalDevice, VK_FORMAT_BC1_RGBA_SRGB_BLOCK, &fmtProps);
		renderState().features[core::enumVal(Feature::TextureCompressionDXT)] =
			(fmtProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;

		// ETC2 compression
		vkGetPhysicalDeviceFormatProperties(s.physicalDevice, VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK, &fmtProps);
		renderState().features[core::enumVal(Feature::TextureCompressionETC2)] =
			(fmtProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;

		// PVRTC compression (not a standard Vulkan 1.0 format - requires IMG extension)
		renderState().features[core::enumVal(Feature::TextureCompressionPVRTC)] = false;
		// ATC compression (AMD-specific, not standard Vulkan)
		renderState().features[core::enumVal(Feature::TextureCompressionATC)] = false;

		// DebugOutput: VK_EXT_debug_utils is widely available
		renderState().features[core::enumVal(Feature::DebugOutput)] = true;

		// BufferStorage: Vulkan persistent mapped memory is always available via HOST_COHERENT memory type
		renderState().features[core::enumVal(Feature::BufferStorage)] = true;

		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(s.physicalDevice, &props);
		renderState().limits[core::enumVal(Limit::MaxComputeWorkGroupCountX)] = (int)props.limits.maxComputeWorkGroupCount[0];
		renderState().limits[core::enumVal(Limit::MaxComputeWorkGroupCountY)] = (int)props.limits.maxComputeWorkGroupCount[1];
		renderState().limits[core::enumVal(Limit::MaxComputeWorkGroupCountZ)] = (int)props.limits.maxComputeWorkGroupCount[2];
		renderState().limits[core::enumVal(Limit::MaxComputeWorkGroupSizeX)] = (int)props.limits.maxComputeWorkGroupSize[0];
		renderState().limits[core::enumVal(Limit::MaxComputeWorkGroupSizeY)] = (int)props.limits.maxComputeWorkGroupSize[1];
		renderState().limits[core::enumVal(Limit::MaxComputeWorkGroupSizeZ)] = (int)props.limits.maxComputeWorkGroupSize[2];
		renderState().limits[core::enumVal(Limit::MaxComputeWorkGroupInvocations)] = (int)props.limits.maxComputeWorkGroupInvocations;
	}

	return true;
}

void activateContext(SDL_Window *window, RendererContext &context) {
}

namespace _priv {

// Forward decls for helpers defined further down in this file.
static void releasePendingDescriptorSets(VkState &s, uint32_t frame);
static void releasePendingFboDestroys(VkState &s);
static void releasePendingBufferDestroys(VkState &s);
static void resetUniformRingFrame(VkState &s, uint32_t frame);

// Record-time: acquire the next swapchain image and start the main render pass so that
// subsequent draw calls (including ImGui) record into a valid, in-progress command buffer.
static bool beginVulkanFrame(VkState &s) {
	if (s.device == VK_NULL_HANDLE || s.swapchain == VK_NULL_HANDLE) {
		return false;
	}
	if (s.frameAcquired) {
		return true;
	}
	if (s.swapchainExtent.width == 0u || s.swapchainExtent.height == 0u) {
		return false;
	}

	// Wait for the previous submission using this frame slot to finish (CPU-GPU sync).
	VkFence inFlight = s.inFlightFences[s.currentFrame];
	vkWaitForFences(s.device, 1, &inFlight, VK_TRUE, UINT64_MAX);

	VkResult ar = vkAcquireNextImageKHR(s.device, s.swapchain, UINT64_MAX, s.imageAvailableSemaphores[s.currentFrame],
										VK_NULL_HANDLE, &s.currentImageIndex);
	if (ar == VK_ERROR_OUT_OF_DATE_KHR) {
		// Swap chain is stale (e.g. window resized). Recreate and try again next frame.
		destroySwapchainResources(s, false);
		if (!createSwapchain(s)) {
			Log::error("Failed to recreate swapchain after OUT_OF_DATE on acquire");
			return false;
		}
		if (!createFramebuffers(s.device, s.renderPass, s.swapchainImageViews, s.glowImageViews, s.swapchainImageCount,
								s.swapchainExtent, s.framebuffers)) {
			Log::error("Failed to recreate framebuffers after OUT_OF_DATE on acquire");
			return false;
		}
		return false;
	}
	if (ar != VK_SUCCESS && ar != VK_SUBOPTIMAL_KHR) {
		Log::error("vkAcquireNextImageKHR failed: %s", vkResultStr(ar));
		return false;
	}

	// Free descriptor sets allocated during the previous iteration of this frame slot.
	// The GPU is guaranteed to be done with them because we just waited on its fence.
	releasePendingDescriptorSets(s, s.currentFrame);
	// Destroy off-screen FBO objects replaced during the last frame(s). Must run before
	// vkResetFences: waitAllInFlightFrames waits every fence, including the current slot
	// which is still signaled here but unsignaled immediately after reset.
	releasePendingFboDestroys(s);
	releasePendingBufferDestroys(s);

	// Only reset the fence after we've successfully acquired, otherwise we'd deadlock
	// the next wait on a frame we never submitted.
	vkResetFences(s.device, 1, &inFlight);
	s.inFlightFencePending[s.currentFrame] = false;

	if (s.frameCommandPools[s.currentFrame] != VK_NULL_HANDLE) {
		vkResetCommandPool(s.device, s.frameCommandPools[s.currentFrame], 0);
	}
	VkCommandBuffer cmd = s.frameCommandBuffers[s.currentFrame];

	VkCommandBufferBeginInfo beginInfo;
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;
	if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
		Log::error("vkBeginCommandBuffer failed");
		return false;
	}
	resetUniformRingFrame(s, s.currentFrame);
	s.boundGraphicsPipeline = VK_NULL_HANDLE;
	s.boundVaoId = InvalidId;
	s.descCacheProgramId = InvalidId;
	s.descCacheSet = VK_NULL_HANDLE;
	s.descCacheImage = InvalidId;
	s.descCacheUboCount = 0u;
	s.descBoundSet = VK_NULL_HANDLE;
	s.descBoundDynOffsetCount = 0u;
	for (uint32_t i = 0; i < 16u; ++i) {
		s.descCacheTextures[i] = InvalidId;
		s.descCacheUboBuffers[i] = VK_NULL_HANDLE;
		s.descCacheUboRanges[i] = 0u;
		s.descBoundDynOffsets[i] = 0u;
	}
	s.descCacheViewport[2] = -1;
	s.descCacheScissor[2] = -1;

	const glm::vec4 &cc = rendererState().pendingClearColor;
	VkClearValue clearValues[2];
	clearValues[0].color.float32[0] = cc.r;
	clearValues[0].color.float32[1] = cc.g;
	clearValues[0].color.float32[2] = cc.b;
	clearValues[0].color.float32[3] = cc.a;
	// Glow attachment: always cleared to transparent black; nothing consumes it.
	clearValues[1].color.float32[0] = 0.0f;
	clearValues[1].color.float32[1] = 0.0f;
	clearValues[1].color.float32[2] = 0.0f;
	clearValues[1].color.float32[3] = 0.0f;

	VkRenderPassBeginInfo rpBegin;
	rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBegin.pNext = nullptr;
	rpBegin.renderPass = s.renderPass;
	rpBegin.framebuffer = s.framebuffers[s.currentImageIndex];
	rpBegin.renderArea.offset.x = 0;
	rpBegin.renderArea.offset.y = 0;
	rpBegin.renderArea.extent = s.swapchainExtent;
	rpBegin.clearValueCount = lengthof(clearValues);
	rpBegin.pClearValues = clearValues;
	vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
	statsRenderPass();

	s.frameAcquired = true;
	s.advancedToOverlaySubpass = false;
	s.mainRenderPassActive = true;
	s.fboRenderPassActive = false;
	s.boundFboHandle = InvalidId;
	s.currentRenderPass = s.renderPass;
	s.currentColorAttachmentCount = 2u; // scene + glow
	s.currentRenderPassHasDepth = false;
	return true;
}

// Forward declarations for the FBO render-pass switching helpers.
static bool buildFboResources(VkState &s, VkState::FramebufferHandle &fb);
static void endFboRenderPassIfActive(VkState &s);
static void endMainRenderPassIfActive(VkState &s);
static void resumeMainRenderPass(VkState &s);
static bool beginFboRenderPass(VkState &s, VkState::FramebufferHandle &fb,
							   FboBeginClear clearMode = FboBeginClear::Auto);

} // namespace _priv

void syncPendingState() {
	_priv::VkState &s = vkstate();
	if (s.device == VK_NULL_HANDLE || s.swapchain == VK_NULL_HANDLE) {
		return;
	}

	// Generic renderer::resize() updates rendererState().windowWidth/windowHeight.
	// Recreate swapchain lazily here when dimensions changed.
	const uint32_t desiredW = (uint32_t)s.windowWidth;
	const uint32_t desiredH = (uint32_t)s.windowHeight;
	if (desiredW != 0u && desiredH != 0u &&
		(s.swapchainExtent.width != desiredW || s.swapchainExtent.height != desiredH)) {
		_priv::destroySwapchainResources(s, false);
		if (!_priv::createSwapchain(s)) {
			Log::error("Failed to recreate swapchain in syncPendingState");
			return;
		}
		if (!_priv::createFramebuffers(s.device, s.renderPass, s.swapchainImageViews, s.glowImageViews,
									   s.swapchainImageCount, s.swapchainExtent, s.framebuffers)) {
			Log::error("Failed to recreate framebuffers in syncPendingState");
			return;
		}
	}

	// Begin the frame up-front so render calls (including ImGui) have a live
	// command buffer inside an active render pass to record into.
	_priv::beginVulkanFrame(s);
}

void endFrame(SDL_Window *window) {
	_priv::VkState &s = vkstate();
	if (!s.frameAcquired) {
		return;
	}

	VkCommandBuffer cmd = s.frameCommandBuffers[s.currentFrame];
	// Viewport rendering may leave an off-screen pass active if unbind was skipped.
	if (s.fboRenderPassActive) {
		_priv::endFboRenderPassIfActive(s);
		_priv::resumeMainRenderPass(s);
	}
	if (!s.mainRenderPassActive) {
		Log::error("endFrame: no active main render pass");
		s.frameAcquired = false;
		return;
	}
	// Vulkan requires every declared subpass to be executed before
	// vkCmdEndRenderPass. If nothing advanced into the overlay subpass this
	// frame (e.g. the application has no ImGui layer) do so here.
	if (!s.advancedToOverlaySubpass) {
		vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
		s.advancedToOverlaySubpass = true;
	}
	vkCmdEndRenderPass(cmd);
	s.mainRenderPassActive = false;
	VkResult er = vkEndCommandBuffer(cmd);
	if (er != VK_SUCCESS) {
		Log::error("vkEndCommandBuffer failed: %s", _priv::vkResultStr(er));
		s.frameAcquired = false;
		return;
	}

	VkSemaphore waitSemaphore = s.imageAvailableSemaphores[s.currentFrame];
	VkSemaphore signalSemaphore = s.renderFinishedSemaphores[s.currentImageIndex];
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submit;
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &waitSemaphore;
	submit.pWaitDstStageMask = &waitStage;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmd;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &signalSemaphore;

	VkResult sr = vkQueueSubmit(s.graphicsQueue, 1, &submit, s.inFlightFences[s.currentFrame]);
	if (sr != VK_SUCCESS) {
		Log::error("vkQueueSubmit failed: %s", _priv::vkResultStr(sr));
		s.frameAcquired = false;
		return;
	}
	s.inFlightFencePending[s.currentFrame] = true;

	VkPresentInfoKHR present;
	present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present.pNext = nullptr;
	present.waitSemaphoreCount = 1;
	present.pWaitSemaphores = &signalSemaphore;
	present.swapchainCount = 1;
	present.pSwapchains = &s.swapchain;
	present.pImageIndices = &s.currentImageIndex;
	present.pResults = nullptr;

	VkResult pr = vkQueuePresentKHR(s.presentQueue, &present);
	if (pr == VK_ERROR_OUT_OF_DATE_KHR || pr == VK_SUBOPTIMAL_KHR) {
		// Will be rebuilt on next syncPendingState / acquire.
		_priv::destroySwapchainResources(s, false);
		if (_priv::createSwapchain(s)) {
			_priv::createFramebuffers(s.device, s.renderPass, s.swapchainImageViews, s.glowImageViews,
									  s.swapchainImageCount, s.swapchainExtent, s.framebuffers);
		}
	} else if (pr != VK_SUCCESS) {
		Log::error("vkQueuePresentKHR failed: %s", _priv::vkResultStr(pr));
	}

	s.currentFrame = (s.currentFrame + 1u) % _priv::MAX_FRAMES_IN_FLIGHT;
	s.frameAcquired = false;
}

void waitDeviceIdle() {
	_priv::VkState &s = vkstate();
	if (s.device == VK_NULL_HANDLE) {
		return;
	}
	// Drop an in-progress recording without submitting so we can wait safely.
	if (s.frameAcquired && s.frameCommandPools[s.currentFrame] != VK_NULL_HANDLE) {
		vkResetCommandPool(s.device, s.frameCommandPools[s.currentFrame], 0);
		s.frameAcquired = false;
		s.mainRenderPassActive = false;
		s.fboRenderPassActive = false;
		s.advancedToOverlaySubpass = false;
		s.inFlightFencePending[s.currentFrame] = false;
	}
	vkDeviceWaitIdle(s.device);
	for (uint32_t i = 0; i < _priv::MAX_FRAMES_IN_FLIGHT; ++i) {
		if (s.frameCommandPools[i] != VK_NULL_HANDLE) {
			vkResetCommandPool(s.device, s.frameCommandPools[i], 0);
		}
		s.inFlightFencePending[i] = false;
	}
}

bool checkError(bool triggerAssert) {
	// Vulkan errors are reported via VkResult at each API call.
	// This stub exists for API compatibility with the OpenGL backend.
	return false;
}

void readBuffer(GBufferTextureType textureType) {
}

float lineWidth(float width) {
	return 1.0f;
}

void clear(ClearFlag flag) {
	_priv::VkState &s = vkstate();
	if (s.device == VK_NULL_HANDLE || !s.frameAcquired) {
		return;
	}
	if ((flag & (ClearFlag::Color | ClearFlag::Depth | ClearFlag::Stencil)) == ClearFlag::None) {
		return;
	}
	if (!s.fboRenderPassActive && !s.mainRenderPassActive) {
		return;
	}
	if (s.mainRenderPassActive && !s.fboRenderPassActive && s.currentRenderPass == s.renderPass) {
		// Main swapchain pass already used LOAD_OP_CLEAR.
		return;
	}
	if (s.fboRenderPassActive && s.boundFboHandle != InvalidId) {
		_priv::VkState::FramebufferHandle *fb = _priv::findFramebufferHandle(s, s.boundFboHandle);
		if (fb != nullptr) {
			const bool wantColor = (flag & ClearFlag::Color) != ClearFlag::None;
			const bool wantDepth = (flag & ClearFlag::Depth) != ClearFlag::None ||
								   (flag & ClearFlag::Stencil) != ClearFlag::None;
			const bool coversAll = (fb->colorCount == 0u || wantColor) &&
								   (fb->depthAttachment == InvalidId || wantDepth);
			if (coversAll) {
				if (s.currentRenderPass == fb->renderPass ||
					s.currentRenderPass == fb->renderPassClearDefined) {
					// Render pass already used LOAD_OP_CLEAR.
					return;
				}
				_priv::endFboRenderPassIfActive(s);
				_priv::beginFboRenderPass(s, *fb, _priv::FboBeginClear::All);
				return;
			}
			if (wantColor && !wantDepth && fb->depthAttachment != InvalidId) {
				if (s.currentRenderPass == fb->renderPassColorClear ||
					s.currentRenderPass == fb->renderPassColorClearDefined) {
					return;
				}
				_priv::endFboRenderPassIfActive(s);
				_priv::beginFboRenderPass(s, *fb, _priv::FboBeginClear::Color);
				return;
			}
		}
	}
	VkCommandBuffer cmd = s.frameCommandBuffers[s.currentFrame];
	const glm::vec4 &cc = rendererState().pendingClearColor;
	VkClearAttachment atts[9];
	uint32_t attCount = 0u;
	uint32_t colorCount = s.currentColorAttachmentCount;
	if (colorCount == 0u) {
		colorCount = s.fboRenderPassActive ? 1u : 2u;
	}
	if ((flag & ClearFlag::Color) != ClearFlag::None) {
		for (uint32_t i = 0; i < colorCount && attCount < lengthof(atts); ++i) {
			atts[attCount].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			atts[attCount].colorAttachment = i;
			atts[attCount].clearValue.color.float32[0] = cc.r;
			atts[attCount].clearValue.color.float32[1] = cc.g;
			atts[attCount].clearValue.color.float32[2] = cc.b;
			atts[attCount].clearValue.color.float32[3] = cc.a;
			++attCount;
		}
	}
	if (s.currentRenderPassHasDepth &&
		((flag & ClearFlag::Depth) != ClearFlag::None || (flag & ClearFlag::Stencil) != ClearFlag::None)) {
		atts[attCount].aspectMask = 0;
		if ((flag & ClearFlag::Depth) != ClearFlag::None) {
			atts[attCount].aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		if ((flag & ClearFlag::Stencil) != ClearFlag::None) {
			atts[attCount].aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		atts[attCount].colorAttachment = 0;
		atts[attCount].clearValue.depthStencil.depth = 1.0f;
		atts[attCount].clearValue.depthStencil.stencil = 0;
		++attCount;
	}
	if (attCount == 0u) {
		return;
	}
	VkClearRect rect;
	rect.rect.offset.x = 0;
	rect.rect.offset.y = 0;
	if (s.fboRenderPassActive && s.boundFboHandle != InvalidId) {
		_priv::VkState::FramebufferHandle *fb = _priv::findFramebufferHandle(s, s.boundFboHandle);
		rect.rect.extent.width = fb != nullptr ? fb->width : s.swapchainExtent.width;
		rect.rect.extent.height = fb != nullptr ? fb->height : s.swapchainExtent.height;
	} else {
		rect.rect.extent = s.swapchainExtent;
	}
	rect.baseArrayLayer = 0;
	rect.layerCount = 1;
	vkCmdClearAttachments(cmd, attCount, atts, 1, &rect);
}

bool bindTexture(TextureUnit unit, TextureType type, Id handle) {
	_priv::VkState &s = vkstate();
	const int u = core::enumVal(unit);
	if (u < 0 || u >= (int)lengthof(s.boundTextures)) {
		return false;
	}
	s.boundTextures[u] = handle;
	return true;
}

bool readTexture(TextureUnit unit, TextureType type, TextureFormat format, Id handle, int w, int h, uint8_t **pixels) {
	return false;
}

namespace _priv {
static int32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeBits, VkMemoryPropertyFlags required,
							  VkMemoryPropertyFlags preferred) {
	VkPhysicalDeviceMemoryProperties props;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &props);
	// Prefer DEVICE_LOCAL|HOST_VISIBLE (ReBAR) when requested so mesh/UBO reads
	// stay in VRAM. First match on required-only often picks sysram HOST_VISIBLE
	// and makes large MCA scenes PCIe-bound (~10 FPS on discrete GPUs).
	if (preferred != 0u) {
		const VkMemoryPropertyFlags want = required | preferred;
		for (uint32_t i = 0; i < props.memoryTypeCount; ++i) {
			if ((typeBits & (1u << i)) == 0) {
				continue;
			}
			if ((props.memoryTypes[i].propertyFlags & want) == want) {
				return (int32_t)i;
			}
		}
	}
	for (uint32_t i = 0; i < props.memoryTypeCount; ++i) {
		if ((typeBits & (1u << i)) == 0) {
			continue;
		}
		if ((props.memoryTypes[i].propertyFlags & required) == required) {
			return (int32_t)i;
		}
	}
	return -1;
}

static bool dedicatedAlloc(VkState &s, VkDeviceSize size, uint32_t memoryTypeBits, VkMemoryPropertyFlags properties,
						   VkDeviceMemory *outMemory, void **outMapped, VkMemoryPropertyFlags preferred) {
	if (outMemory == nullptr) {
		return false;
	}
	// Prefer ReBAR (DEVICE_LOCAL|HOST_VISIBLE) when asked, but fall back to
	// plain HOST_VISIBLE sysram if that heap is exhausted.
	const VkMemoryPropertyFlags prefs[2] = {preferred, 0};
	const uint32_t passes = preferred != 0u ? 2u : 1u;
	for (uint32_t pass = 0; pass < passes; ++pass) {
		const int32_t memType = findMemoryType(s.physicalDevice, memoryTypeBits, properties, prefs[pass]);
		if (memType < 0) {
			continue;
		}
		VkMemoryAllocateInfo ai{};
		ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		ai.allocationSize = size;
		ai.memoryTypeIndex = (uint32_t)memType;
		if (vkAllocateMemory(s.device, &ai, nullptr, outMemory) != VK_SUCCESS) {
			*outMemory = VK_NULL_HANDLE;
			continue;
		}
		if (outMapped != nullptr) {
			*outMapped = nullptr;
			if ((properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
				if (vkMapMemory(s.device, *outMemory, 0, VK_WHOLE_SIZE, 0, outMapped) != VK_SUCCESS) {
					vkFreeMemory(s.device, *outMemory, nullptr);
					*outMemory = VK_NULL_HANDLE;
					continue;
				}
			}
		}
		return true;
	}
	return false;
}

static bool ensureSlabCapacity(VkState &s) {
	if (s.memorySlabCount < s.memorySlabCapacity) {
		return true;
	}
	const uint32_t oldCapacity = s.memorySlabCapacity;
	const uint32_t newCapacity = oldCapacity == 0u ? 32u : oldCapacity * 2u;
	VkState::MemorySlab *neu = new VkState::MemorySlab[newCapacity];
	if (neu == nullptr) {
		return false;
	}
	if (s.memorySlabs != nullptr) {
		for (uint32_t i = 0; i < oldCapacity; ++i) {
			neu[i] = s.memorySlabs[i];
		}
		delete[] s.memorySlabs;
	}
	s.memorySlabs = neu;
	s.memorySlabCapacity = newCapacity;
	return true;
}

static bool slabAlloc(VkState &s, VkDeviceSize size, VkDeviceSize alignment, uint32_t memoryTypeBits,
					  VkMemoryPropertyFlags properties, VkDeviceMemory *outMemory, VkDeviceSize *outOffset,
					  void **outMapped, VkMemoryPropertyFlags preferred) {
	if (s.device == VK_NULL_HANDLE || size == 0u || outMemory == nullptr || outOffset == nullptr) {
		return false;
	}
	if (alignment < 256u) {
		alignment = 256u;
	}
	if (size + alignment > VkState::MEMORY_SLAB_SIZE) {
		return false;
	}
	// Prefer ReBAR when requested; if that heap cannot fit another 64MiB slab,
	// fall back to host-visible sysram so mesh uploads keep working.
	const VkMemoryPropertyFlags prefs[2] = {preferred, 0};
	const uint32_t passes = preferred != 0u ? 2u : 1u;
	for (uint32_t pass = 0; pass < passes; ++pass) {
		const int32_t memType = findMemoryType(s.physicalDevice, memoryTypeBits, properties, prefs[pass]);
		if (memType < 0) {
			continue;
		}
		for (uint32_t i = 0; i < s.memorySlabCount; ++i) {
			VkState::MemorySlab &slab = s.memorySlabs[i];
			if (slab.memoryTypeIndex != (uint32_t)memType || slab.memory == VK_NULL_HANDLE) {
				continue;
			}
			const VkDeviceSize aligned = (slab.used + alignment - 1u) & ~(alignment - 1u);
			if (aligned + size <= slab.capacity) {
				*outMemory = slab.memory;
				*outOffset = aligned;
				if (outMapped != nullptr) {
					*outMapped = (slab.mapped != nullptr) ? ((uint8_t *)slab.mapped + aligned) : nullptr;
				}
				slab.used = aligned + size;
				return true;
			}
		}
		if (!ensureSlabCapacity(s)) {
			Log::error("Vulkan memory slab grow failed");
			return false;
		}
		VkMemoryAllocateInfo ai{};
		ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		ai.allocationSize = VkState::MEMORY_SLAB_SIZE;
		ai.memoryTypeIndex = (uint32_t)memType;
		VkState::MemorySlab &slab = s.memorySlabs[s.memorySlabCount];
		if (vkAllocateMemory(s.device, &ai, nullptr, &slab.memory) != VK_SUCCESS) {
			slab.memory = VK_NULL_HANDLE;
			Log::debug("Vulkan memory slab alloc failed for memoryType %d (pass %u)", memType, pass);
			continue;
		}
		slab.capacity = VkState::MEMORY_SLAB_SIZE;
		slab.used = 0;
		slab.memoryTypeIndex = (uint32_t)memType;
		slab.properties = properties;
		slab.mapped = nullptr;
		if ((properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
			if (vkMapMemory(s.device, slab.memory, 0, VK_WHOLE_SIZE, 0, &slab.mapped) != VK_SUCCESS) {
				Log::error("Failed to map memory slab");
				vkFreeMemory(s.device, slab.memory, nullptr);
				slab.memory = VK_NULL_HANDLE;
				continue;
			}
		}
		++s.memorySlabCount;
		*outMemory = slab.memory;
		*outOffset = 0;
		if (outMapped != nullptr) {
			*outMapped = slab.mapped;
		}
		slab.used = size;
		return true;
	}
	Log::error("Failed to allocate %llu byte memory slab", (unsigned long long)VkState::MEMORY_SLAB_SIZE);
	return false;
}

static bool allocGpuMemory(VkState &s, VkDeviceSize size, VkDeviceSize alignment, uint32_t memoryTypeBits,
						   VkMemoryPropertyFlags properties, VkDeviceMemory *outMemory, VkDeviceSize *outOffset,
						   void **outMapped, bool *outSlab, VkMemoryPropertyFlags preferred) {
	if (size + (alignment > 256u ? alignment : 256u) > VkState::MEMORY_SLAB_SIZE) {
		if (outSlab != nullptr) {
			*outSlab = false;
		}
		if (outOffset != nullptr) {
			*outOffset = 0;
		}
		return dedicatedAlloc(s, size, memoryTypeBits, properties, outMemory, outMapped, preferred);
	}
	if (outSlab != nullptr) {
		*outSlab = true;
	}
	return slabAlloc(s, size, alignment, memoryTypeBits, properties, outMemory, outOffset, outMapped, preferred);
}

static void destroyMemorySlabs(VkState &s) {
	if (s.device != VK_NULL_HANDLE) {
		for (uint32_t i = 0; i < s.memorySlabCount; ++i) {
			VkState::MemorySlab &slab = s.memorySlabs[i];
			if (slab.mapped != nullptr && slab.memory != VK_NULL_HANDLE) {
				vkUnmapMemory(s.device, slab.memory);
				slab.mapped = nullptr;
			}
			if (slab.memory != VK_NULL_HANDLE) {
				vkFreeMemory(s.device, slab.memory, nullptr);
				slab.memory = VK_NULL_HANDLE;
			}
			slab.capacity = 0;
			slab.used = 0;
		}
	}
	delete[] s.memorySlabs;
	s.memorySlabs = nullptr;
	s.memorySlabCount = 0u;
	s.memorySlabCapacity = 0u;
}

static VkBufferUsageFlags bufferUsageFor(BufferType type) {
	VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	switch (type) {
	case BufferType::ArrayBuffer:
		usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		break;
	case BufferType::IndexBuffer:
		usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		break;
	case BufferType::UniformBuffer:
		return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	case BufferType::ShaderStorageBuffer:
		usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		break;
	case BufferType::IndirectBuffer:
		usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		break;
	case BufferType::PixelBuffer:
	case BufferType::TransformBuffer:
	case BufferType::Max:
		usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		break;
	}
	// Mesh-like buffers live in DEVICE_LOCAL memory and are filled via staging.
	usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	return usage;
}

static bool bufferUseDeviceLocal(BufferType type) {
	return type == BufferType::ArrayBuffer || type == BufferType::IndexBuffer ||
		   type == BufferType::ShaderStorageBuffer || type == BufferType::IndirectBuffer;
}

static void destroyBufferAlloc(VkState &s, VkState::BufferHandle &h) {
	if (h.buffer == VK_NULL_HANDLE && h.memory == VK_NULL_HANDLE && h.mapped == nullptr) {
		h.capacity = 0u;
		h.slabMemory = false;
		h.usage = 0u;
		return;
	}
	// Defer GPU destroy: the old VkBuffer may still be referenced by an in-flight
	// or currently recording command buffer (common when meshes grow on upload).
	if (s.pendingBufferDestroyCount >= s.pendingBufferDestroyCapacity) {
		const uint32_t newCap = s.pendingBufferDestroyCapacity == 0u ? 64u : s.pendingBufferDestroyCapacity * 2u;
		VkState::PendingBufferDestroy *newArr =
			(VkState::PendingBufferDestroy *)core_malloc(sizeof(VkState::PendingBufferDestroy) * newCap);
		if (s.pendingBufferDestroys != nullptr) {
			core_memcpy(newArr, s.pendingBufferDestroys,
						sizeof(VkState::PendingBufferDestroy) * s.pendingBufferDestroyCount);
			core_free(s.pendingBufferDestroys);
		}
		s.pendingBufferDestroys = newArr;
		s.pendingBufferDestroyCapacity = newCap;
	}
	VkState::PendingBufferDestroy &p = s.pendingBufferDestroys[s.pendingBufferDestroyCount++];
	p.buffer = h.buffer;
	p.memory = h.memory;
	p.mapped = h.mapped;
	p.freeMemory = h.memory != VK_NULL_HANDLE && !h.slabMemory;
	h.buffer = VK_NULL_HANDLE;
	h.memory = VK_NULL_HANDLE;
	h.mapped = nullptr;
	h.slabMemory = false;
	h.capacity = 0u;
	h.usage = 0u;
}

// Create or grow the VkBuffer + backing memory behind @c h so that it can hold
// @c size bytes with the usage implied by @c h.type. Returns false on failure.
// Vertex/index/SSBO/indirect buffers use DEVICE_LOCAL memory (filled via staging).
// Uniform buffers stay HOST_VISIBLE (preferring ReBAR) and persistently mapped.
// Existing contents are not preserved across a grow.
static bool ensureBufferAlloc(VkState &s, VkState::BufferHandle &h, size_t size) {
	if (s.device == VK_NULL_HANDLE || size == 0u) {
		return false;
	}
	const VkBufferUsageFlags usage = bufferUsageFor(h.type);
	if (h.buffer != VK_NULL_HANDLE && size <= (size_t)h.capacity && h.usage == usage) {
		return true;
	}
	// Reallocate.
	destroyBufferAlloc(s, h);
	h.usage = usage;

	VkBufferCreateInfo bi;
	bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bi.pNext = nullptr;
	bi.flags = 0;
	bi.size = (VkDeviceSize)size;
	bi.usage = usage;
	bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bi.queueFamilyIndexCount = 0;
	bi.pQueueFamilyIndices = nullptr;
	if (vkCreateBuffer(s.device, &bi, nullptr, &h.buffer) != VK_SUCCESS) {
		Log::error("vkCreateBuffer failed (size=%zu)", size);
		return false;
	}

	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(s.device, h.buffer, &req);
	VkDeviceSize offset = 0;
	void *mapped = nullptr;
	bool slab = true;
	const bool deviceLocal = bufferUseDeviceLocal(h.type);
	if (deviceLocal) {
		if (!allocGpuMemory(s, req.size, req.alignment, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
							&h.memory, &offset, nullptr, &slab, 0)) {
			Log::error("No device-local memory for buffer (size=%zu)", size);
			vkDestroyBuffer(s.device, h.buffer, nullptr);
			h.buffer = VK_NULL_HANDLE;
			return false;
		}
		mapped = nullptr;
	} else if (!allocGpuMemory(s, req.size, req.alignment, req.memoryTypeBits,
							   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &h.memory,
							   &offset, &mapped, &slab, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
		Log::error("No host-visible memory for buffer (size=%zu)", size);
		vkDestroyBuffer(s.device, h.buffer, nullptr);
		h.buffer = VK_NULL_HANDLE;
		return false;
	}
	if (vkBindBufferMemory(s.device, h.buffer, h.memory, offset) != VK_SUCCESS) {
		Log::error("vkBindBufferMemory failed");
		vkDestroyBuffer(s.device, h.buffer, nullptr);
		h.buffer = VK_NULL_HANDLE;
		if (!slab && h.memory != VK_NULL_HANDLE) {
			if (mapped != nullptr) {
				vkUnmapMemory(s.device, h.memory);
			}
			vkFreeMemory(s.device, h.memory, nullptr);
		}
		h.memory = VK_NULL_HANDLE;
		return false;
	}
	h.mapped = mapped;
	h.slabMemory = slab;
	h.capacity = req.size;
	return true;
}

// Upload bytes into @p h at @p offset. Host-visible buffers memcpy; DEVICE_LOCAL
// buffers go through a staging buffer + vkCmdCopyBuffer.
static bool uploadBufferRegion(VkState &s, VkState::BufferHandle &h, size_t offset, const void *data, size_t size) {
	if (data == nullptr || size == 0u || h.buffer == VK_NULL_HANDLE) {
		return data == nullptr || size == 0u;
	}
	if (offset + size > (size_t)h.capacity) {
		return false;
	}
	if (h.mapped != nullptr) {
		core_memcpy((uint8_t *)h.mapped + offset, data, size);
		return true;
	}

	VkBuffer staging = VK_NULL_HANDLE;
	VkDeviceMemory stagingMem = VK_NULL_HANDLE;
	void *stagingPtr = nullptr;
	VkBufferCreateInfo bci{};
	bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bci.size = (VkDeviceSize)size;
	bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(s.device, &bci, nullptr, &staging) != VK_SUCCESS) {
		Log::error("uploadBufferRegion: staging create failed");
		return false;
	}
	VkMemoryRequirements bmr;
	vkGetBufferMemoryRequirements(s.device, staging, &bmr);
	if (!dedicatedAlloc(s, bmr.size, bmr.memoryTypeBits,
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingMem,
						&stagingPtr, 0) ||
		stagingPtr == nullptr) {
		Log::error("uploadBufferRegion: staging alloc failed");
		vkDestroyBuffer(s.device, staging, nullptr);
		return false;
	}
	if (vkBindBufferMemory(s.device, staging, stagingMem, 0) != VK_SUCCESS) {
		Log::error("uploadBufferRegion: staging bind failed");
		if (stagingPtr != nullptr) {
			vkUnmapMemory(s.device, stagingMem);
		}
		vkFreeMemory(s.device, stagingMem, nullptr);
		vkDestroyBuffer(s.device, staging, nullptr);
		return false;
	}
	core_memcpy(stagingPtr, data, size);

	VkBufferCopy region{};
	region.srcOffset = 0;
	region.dstOffset = (VkDeviceSize)offset;
	region.size = (VkDeviceSize)size;

	if (s.frameAcquired && s.frameCommandBuffers[s.currentFrame] != VK_NULL_HANDLE) {
		endFboRenderPassIfActive(s);
		endMainRenderPassIfActive(s);
		VkCommandBuffer cmd = s.frameCommandBuffers[s.currentFrame];
		VkBufferMemoryBarrier barriers[2]{};
		barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barriers[0].srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[0].buffer = staging;
		barriers[0].offset = 0;
		barriers[0].size = VK_WHOLE_SIZE;
		barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barriers[1].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
									 VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[1].buffer = h.buffer;
		barriers[1].offset = (VkDeviceSize)offset;
		barriers[1].size = (VkDeviceSize)size;
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
									  VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 2, barriers, 0, nullptr);
		vkCmdCopyBuffer(cmd, staging, h.buffer, 1, &region);
		VkBufferMemoryBarrier after{};
		after.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		after.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		after.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT |
							  VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		after.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		after.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		after.buffer = h.buffer;
		after.offset = (VkDeviceSize)offset;
		after.size = (VkDeviceSize)size;
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
							 VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
								 VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
							 0, 0, nullptr, 1, &after, 0, nullptr);
		// Staging must stay alive until this frame finishes.
		if (s.pendingBufferDestroyCount >= s.pendingBufferDestroyCapacity) {
			const uint32_t newCap = s.pendingBufferDestroyCapacity == 0u ? 64u : s.pendingBufferDestroyCapacity * 2u;
			VkState::PendingBufferDestroy *newArr =
				(VkState::PendingBufferDestroy *)core_malloc(sizeof(VkState::PendingBufferDestroy) * newCap);
			if (s.pendingBufferDestroys != nullptr) {
				core_memcpy(newArr, s.pendingBufferDestroys,
							sizeof(VkState::PendingBufferDestroy) * s.pendingBufferDestroyCount);
				core_free(s.pendingBufferDestroys);
			}
			s.pendingBufferDestroys = newArr;
			s.pendingBufferDestroyCapacity = newCap;
		}
		VkState::PendingBufferDestroy &pend = s.pendingBufferDestroys[s.pendingBufferDestroyCount++];
		pend.buffer = staging;
		pend.memory = stagingMem;
		pend.mapped = stagingPtr;
		pend.freeMemory = true;
		return true;
	}

	// Outside a frame: one-shot submit.
	if (s.commandPool == VK_NULL_HANDLE) {
		vkUnmapMemory(s.device, stagingMem);
		vkFreeMemory(s.device, stagingMem, nullptr);
		vkDestroyBuffer(s.device, staging, nullptr);
		return false;
	}
	VkCommandBufferAllocateInfo cbai{};
	cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbai.commandPool = s.commandPool;
	cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cbai.commandBufferCount = 1;
	VkCommandBuffer ccb = VK_NULL_HANDLE;
	if (vkAllocateCommandBuffers(s.device, &cbai, &ccb) != VK_SUCCESS || ccb == VK_NULL_HANDLE) {
		vkUnmapMemory(s.device, stagingMem);
		vkFreeMemory(s.device, stagingMem, nullptr);
		vkDestroyBuffer(s.device, staging, nullptr);
		return false;
	}
	VkCommandBufferBeginInfo cbbi{};
	cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(ccb, &cbbi);
	vkCmdCopyBuffer(ccb, staging, h.buffer, 1, &region);
	VkBufferMemoryBarrier after{};
	after.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	after.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	after.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
	after.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	after.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	after.buffer = h.buffer;
	after.offset = (VkDeviceSize)offset;
	after.size = (VkDeviceSize)size;
	vkCmdPipelineBarrier(ccb, VK_PIPELINE_STAGE_TRANSFER_BIT,
						 VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 1,
						 &after, 0, nullptr);
	vkEndCommandBuffer(ccb);
	VkSubmitInfo si{};
	si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	si.commandBufferCount = 1;
	si.pCommandBuffers = &ccb;
	vkQueueSubmit(s.graphicsQueue, 1, &si, VK_NULL_HANDLE);
	vkQueueWaitIdle(s.graphicsQueue);
	vkFreeCommandBuffers(s.device, s.commandPool, 1, &ccb);
	vkUnmapMemory(s.device, stagingMem);
	vkFreeMemory(s.device, stagingMem, nullptr);
	vkDestroyBuffer(s.device, staging, nullptr);
	return true;
}

// ----------------------------------------------------------------------------
// Off-screen framebuffer support: build a VkRenderPass + VkFramebuffer matching
// the texture attachments and switch render passes mid-frame on bindFramebuffer.
// ----------------------------------------------------------------------------

static void waitAllInFlightFrames(VkState &s) {
	if (s.device == VK_NULL_HANDLE) {
		return;
	}
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		if (s.inFlightFences[i] == VK_NULL_HANDLE || !s.inFlightFencePending[i]) {
			continue;
		}
		// Recording into this slot: the fence was reset and has no submit yet.
		if (s.frameAcquired && i == s.currentFrame) {
			continue;
		}
		vkWaitForFences(s.device, 1, &s.inFlightFences[i], VK_TRUE, UINT64_MAX);
	}
}

static void queuePendingFboDestroy(VkState &s, VkState::FramebufferHandle &fb) {
	if (fb.framebuffer == VK_NULL_HANDLE && fb.renderPass == VK_NULL_HANDLE && fb.renderPassLoad == VK_NULL_HANDLE &&
		fb.renderPassClearDefined == VK_NULL_HANDLE && fb.renderPassColorClear == VK_NULL_HANDLE &&
		fb.renderPassColorClearDefined == VK_NULL_HANDLE &&
		fb.depthLayerView == VK_NULL_HANDLE) {
		return;
	}
	if (s.pendingFboDestroyCount >= s.pendingFboDestroyCapacity) {
		const uint32_t newCap = s.pendingFboDestroyCapacity == 0u ? 16u : s.pendingFboDestroyCapacity * 2u;
		VkState::PendingFboDestroy *newArr =
			(VkState::PendingFboDestroy *)core_malloc(sizeof(VkState::PendingFboDestroy) * newCap);
		if (s.pendingFboDestroys != nullptr) {
			core_memcpy(newArr, s.pendingFboDestroys,
						sizeof(VkState::PendingFboDestroy) * s.pendingFboDestroyCount);
			core_free(s.pendingFboDestroys);
		}
		s.pendingFboDestroys = newArr;
		s.pendingFboDestroyCapacity = newCap;
	}
	VkState::PendingFboDestroy &p = s.pendingFboDestroys[s.pendingFboDestroyCount++];
	p.framebuffer = fb.framebuffer;
	p.renderPass = fb.renderPass;
	p.renderPassLoad = fb.renderPassLoad;
	p.renderPassClearDefined = fb.renderPassClearDefined;
	p.renderPassColorClear = fb.renderPassColorClear;
	p.renderPassColorClearDefined = fb.renderPassColorClearDefined;
	p.extraView = fb.depthLayerView;
	fb.framebuffer = VK_NULL_HANDLE;
	fb.renderPass = VK_NULL_HANDLE;
	fb.renderPassLoad = VK_NULL_HANDLE;
	fb.renderPassClearDefined = VK_NULL_HANDLE;
	fb.renderPassColorClear = VK_NULL_HANDLE;
	fb.renderPassColorClearDefined = VK_NULL_HANDLE;
	fb.depthLayerView = VK_NULL_HANDLE;
}

static void releasePendingFboDestroys(VkState &s) {
	if (s.pendingFboDestroyCount == 0u || s.device == VK_NULL_HANDLE) {
		return;
	}
	waitAllInFlightFrames(s);
	for (uint32_t i = 0; i < s.pendingFboDestroyCount; ++i) {
		const VkState::PendingFboDestroy &p = s.pendingFboDestroys[i];
		if (p.framebuffer != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(s.device, p.framebuffer, nullptr);
		}
		if (p.renderPass != VK_NULL_HANDLE) {
			vkDestroyRenderPass(s.device, p.renderPass, nullptr);
		}
		if (p.renderPassLoad != VK_NULL_HANDLE) {
			vkDestroyRenderPass(s.device, p.renderPassLoad, nullptr);
		}
		if (p.renderPassClearDefined != VK_NULL_HANDLE) {
			vkDestroyRenderPass(s.device, p.renderPassClearDefined, nullptr);
		}
		if (p.renderPassColorClear != VK_NULL_HANDLE) {
			vkDestroyRenderPass(s.device, p.renderPassColorClear, nullptr);
		}
		if (p.renderPassColorClearDefined != VK_NULL_HANDLE) {
			vkDestroyRenderPass(s.device, p.renderPassColorClearDefined, nullptr);
		}
		if (p.extraView != VK_NULL_HANDLE) {
			vkDestroyImageView(s.device, p.extraView, nullptr);
		}
	}
	s.pendingFboDestroyCount = 0u;
}

static void releasePendingBufferDestroys(VkState &s) {
	if (s.pendingBufferDestroyCount == 0u || s.device == VK_NULL_HANDLE) {
		return;
	}
	waitAllInFlightFrames(s);
	for (uint32_t i = 0; i < s.pendingBufferDestroyCount; ++i) {
		const VkState::PendingBufferDestroy &p = s.pendingBufferDestroys[i];
		if (p.mapped != nullptr && p.memory != VK_NULL_HANDLE && p.freeMemory) {
			vkUnmapMemory(s.device, p.memory);
		}
		if (p.buffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(s.device, p.buffer, nullptr);
		}
		if (p.freeMemory && p.memory != VK_NULL_HANDLE) {
			vkFreeMemory(s.device, p.memory, nullptr);
		}
	}
	s.pendingBufferDestroyCount = 0u;
}

static void queuePendingFramebufferAndView(VkState &s, VkFramebuffer framebuffer, VkImageView extraView) {
	if (framebuffer == VK_NULL_HANDLE && extraView == VK_NULL_HANDLE) {
		return;
	}
	if (s.pendingFboDestroyCount >= s.pendingFboDestroyCapacity) {
		const uint32_t newCap = s.pendingFboDestroyCapacity == 0u ? 16u : s.pendingFboDestroyCapacity * 2u;
		VkState::PendingFboDestroy *newArr =
			(VkState::PendingFboDestroy *)core_malloc(sizeof(VkState::PendingFboDestroy) * newCap);
		if (s.pendingFboDestroys != nullptr) {
			core_memcpy(newArr, s.pendingFboDestroys,
						sizeof(VkState::PendingFboDestroy) * s.pendingFboDestroyCount);
			core_free(s.pendingFboDestroys);
		}
		s.pendingFboDestroys = newArr;
		s.pendingFboDestroyCapacity = newCap;
	}
	VkState::PendingFboDestroy &p = s.pendingFboDestroys[s.pendingFboDestroyCount++];
	p.framebuffer = framebuffer;
	p.renderPass = VK_NULL_HANDLE;
	p.renderPassLoad = VK_NULL_HANDLE;
	p.renderPassClearDefined = VK_NULL_HANDLE;
	p.renderPassColorClear = VK_NULL_HANDLE;
	p.renderPassColorClearDefined = VK_NULL_HANDLE;
	p.extraView = extraView;
}

static bool createDepthLayerView(VkState &s, VkState::FramebufferHandle &fb, const VkState::TextureHandle &th,
								 VkImageView *outView) {
	if (th.layers <= 1) {
		*outView = th.view;
		return th.view != VK_NULL_HANDLE;
	}
	VkImageViewCreateInfo ivci{};
	ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ivci.image = th.image;
	ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
	ivci.format = textureFormatToVk(th.format);
	ivci.subresourceRange.aspectMask = imageAspectForFormat(th.format);
	ivci.subresourceRange.baseMipLevel = 0;
	ivci.subresourceRange.levelCount = 1;
	const int layer = core_max(0, fb.depthLayer);
	ivci.subresourceRange.baseArrayLayer = (uint32_t)layer;
	ivci.subresourceRange.layerCount = 1;
	if (vkCreateImageView(s.device, &ivci, nullptr, &fb.depthLayerView) != VK_SUCCESS) {
		Log::error("FBO %u: failed to create depth layer view for layer %d", fb.id, layer);
		fb.depthLayerView = VK_NULL_HANDLE;
		return false;
	}
	*outView = fb.depthLayerView;
	return true;
}

static bool buildFboResources(VkState &s, VkState::FramebufferHandle &fb) {
	if (!fb.dirty && fb.framebuffer != VK_NULL_HANDLE) {
		return true;
	}
	if (fb.colorCount == 0u && fb.depthAttachment == InvalidId) {
		return false;
	}
	// Resolve attachment views/extent from the texture handles.
	uint32_t width = 0, height = 0;
	VkImageView views[9]; // up to 8 color + 1 depth
	uint32_t viewCount = 0;
	for (uint32_t i = 0; i < fb.colorCount; ++i) {
		VkState::TextureHandle *th = findTextureHandle(s, fb.colorAttachments[i]);
		if (th == nullptr || th->view == VK_NULL_HANDLE) {
			Log::error("FBO %u: color attachment %u has no view", fb.id, i);
			return false;
		}
		if (i == 0) { width = (uint32_t)th->width; height = (uint32_t)th->height; }
		fb.colorFormats[i] = textureFormatToVk(th->format);
		views[viewCount++] = th->view;
	}
	VkState::TextureHandle *depthTex = nullptr;
	if (fb.depthAttachment != InvalidId) {
		depthTex = findTextureHandle(s, fb.depthAttachment);
		if (depthTex == nullptr || depthTex->view == VK_NULL_HANDLE) {
			Log::error("FBO %u: depth attachment has no view", fb.id);
			return false;
		}
		if (width == 0) { width = (uint32_t)depthTex->width; height = (uint32_t)depthTex->height; }
		fb.depthFormat = textureFormatToVk(depthTex->format);
	}
	fb.width = width;
	fb.height = height;

	// End an active render pass that still references the old objects before we
	// replace them (subsequent draws will use the newly created resources).
	if (s.frameAcquired && s.fboRenderPassActive && s.boundFboHandle == fb.id) {
		endFboRenderPassIfActive(s);
	}

	// Queue previous resources for deferred destroy. They may still be referenced
	// by a submitted or currently recording command buffer.
	if (fb.framebuffer != VK_NULL_HANDLE || fb.renderPass != VK_NULL_HANDLE ||
		fb.renderPassLoad != VK_NULL_HANDLE || fb.renderPassClearDefined != VK_NULL_HANDLE ||
		fb.renderPassColorClear != VK_NULL_HANDLE || fb.renderPassColorClearDefined != VK_NULL_HANDLE ||
		fb.depthLayerView != VK_NULL_HANDLE) {
		queuePendingFboDestroy(s, fb);
	}

	if (depthTex != nullptr) {
		VkImageView depthView = VK_NULL_HANDLE;
		if (!createDepthLayerView(s, fb, *depthTex, &depthView)) {
			return false;
		}
		views[viewCount++] = depthView;
	}

	// Build render pass(es): first-use CLEAR (undefined layout), LOAD for
	// re-entry, CLEAR with defined layouts, and color-only CLEAR (depth LOAD)
	// for OIT bind(false)+clear(Color) after a depth blit.
	auto buildRenderPass = [&](VkAttachmentLoadOp colorLoadOp, VkAttachmentLoadOp depthLoadOp, bool colorDefined,
							   bool depthDefinedLayout, VkRenderPass *outPass) -> bool {
		VkAttachmentDescription atts[9]{};
		VkAttachmentReference colorRefs[8]{};
		VkAttachmentReference depthRef{};
		bool hasDepth = (fb.depthAttachment != InvalidId);
		uint32_t attCount = 0;
		for (uint32_t i = 0; i < fb.colorCount; ++i) {
			atts[attCount].format = fb.colorFormats[i];
			atts[attCount].samples = VK_SAMPLE_COUNT_1_BIT;
			atts[attCount].loadOp = colorLoadOp;
			atts[attCount].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			atts[attCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			atts[attCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			atts[attCount].initialLayout = colorDefined
				? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
			atts[attCount].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			colorRefs[i].attachment = attCount;
			colorRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			++attCount;
		}
		if (hasDepth) {
			atts[attCount].format = fb.depthFormat;
			atts[attCount].samples = VK_SAMPLE_COUNT_1_BIT;
			atts[attCount].loadOp = depthLoadOp;
			atts[attCount].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			atts[attCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			atts[attCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			atts[attCount].initialLayout = depthDefinedLayout
				? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
			atts[attCount].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			depthRef.attachment = attCount;
			depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			++attCount;
		}
		VkSubpassDescription sub{};
		sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		sub.colorAttachmentCount = fb.colorCount;
		sub.pColorAttachments = fb.colorCount > 0 ? colorRefs : nullptr;
		sub.pDepthStencilAttachment = hasDepth ? &depthRef : nullptr;
		VkSubpassDependency deps[2]{};
		deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		deps[0].dstSubpass = 0;
		deps[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		deps[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		deps[1].srcSubpass = 0;
		deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		deps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		deps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		if (hasDepth) {
			deps[0].dstStageMask |=
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			deps[0].dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
									 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			deps[1].srcStageMask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			deps[1].srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		}
		VkRenderPassCreateInfo rpci{};
		rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rpci.attachmentCount = attCount;
		rpci.pAttachments = atts;
		rpci.subpassCount = 1;
		rpci.pSubpasses = &sub;
		rpci.dependencyCount = 2;
		rpci.pDependencies = deps;
		return vkCreateRenderPass(s.device, &rpci, nullptr, outPass) == VK_SUCCESS;
	};
	if (!buildRenderPass(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_LOAD_OP_CLEAR, false, false, &fb.renderPass)) {
		Log::error("FBO %u: vkCreateRenderPass (clear) failed", fb.id);
		return false;
	}
	if (!buildRenderPass(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_LOAD, true, true, &fb.renderPassLoad)) {
		Log::error("FBO %u: vkCreateRenderPass (load) failed", fb.id);
		return false;
	}
	if (!buildRenderPass(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_LOAD_OP_CLEAR, true, true, &fb.renderPassClearDefined)) {
		Log::error("FBO %u: vkCreateRenderPass (clear-defined) failed", fb.id);
		return false;
	}
	if (fb.colorCount > 0u && fb.depthAttachment != InvalidId) {
		if (!buildRenderPass(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_LOAD_OP_LOAD, false, true, &fb.renderPassColorClear)) {
			Log::error("FBO %u: vkCreateRenderPass (color-clear) failed", fb.id);
			return false;
		}
		if (!buildRenderPass(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_LOAD_OP_LOAD, true, true, &fb.renderPassColorClearDefined)) {
			Log::error("FBO %u: vkCreateRenderPass (color-clear-defined) failed", fb.id);
			return false;
		}
	}

	VkFramebufferCreateInfo fci{};
	fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fci.renderPass = fb.renderPass;
	fci.attachmentCount = viewCount;
	fci.pAttachments = views;
	fci.width = width;
	fci.height = height;
	fci.layers = 1;
	if (vkCreateFramebuffer(s.device, &fci, nullptr, &fb.framebuffer) != VK_SUCCESS) {
		Log::error("FBO %u: vkCreateFramebuffer failed", fb.id);
		return false;
	}
	fb.dirty = false;
	return true;
}

static void endMainRenderPassIfActive(VkState &s) {
	if (!s.mainRenderPassActive || !s.frameAcquired) {
		return;
	}
	VkCommandBuffer cmd = s.frameCommandBuffers[s.currentFrame];
	if (!s.advancedToOverlaySubpass) {
		vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
		s.advancedToOverlaySubpass = true;
	}
	vkCmdEndRenderPass(cmd);
	s.mainRenderPassActive = false;
	s.currentRenderPass = VK_NULL_HANDLE;
	s.currentColorAttachmentCount = 0u;
}

static void endFboRenderPassIfActive(VkState &s) {
	if (!s.fboRenderPassActive || !s.frameAcquired) {
		return;
	}
	if (s.boundFboHandle != InvalidId) {
		VkState::FramebufferHandle *fb = findFramebufferHandle(s, s.boundFboHandle);
		if (fb != nullptr) {
			if (fb->depthAttachment != InvalidId) {
				VkState::TextureHandle *th = findTextureHandle(s, fb->depthAttachment);
				if (th != nullptr) {
					th->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				}
			}
			for (uint32_t i = 0; i < fb->colorCount; ++i) {
				VkState::TextureHandle *cth = findTextureHandle(s, fb->colorAttachments[i]);
				if (cth != nullptr) {
					cth->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				}
			}
		}
	}
	VkCommandBuffer cmd = s.frameCommandBuffers[s.currentFrame];
	vkCmdEndRenderPass(cmd);
	s.fboRenderPassActive = false;
	s.boundFboHandle = InvalidId;
	s.currentRenderPass = VK_NULL_HANDLE;
	s.currentColorAttachmentCount = 0u;
	s.currentRenderPassHasDepth = false;
}

static void resumeMainRenderPass(VkState &s) {
	if (s.mainRenderPassActive || !s.frameAcquired) {
		return;
	}
	VkCommandBuffer cmd = s.frameCommandBuffers[s.currentFrame];
	// We need a LOAD_OP_LOAD variant of the main render pass. Build one
	// lazily and cache it on the state.
	if (s.mainResumeRenderPass == VK_NULL_HANDLE) {
		// Mirror createRenderPass() but with LOAD_OP_LOAD.
		VkAttachmentDescription atts[3]{};
		atts[0].format = s.swapchainFormat;
		atts[0].samples = VK_SAMPLE_COUNT_1_BIT;
		atts[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		atts[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		atts[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		atts[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		atts[0].initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		atts[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		atts[1].format = s.glowFormat;
		atts[1].samples = VK_SAMPLE_COUNT_1_BIT;
		atts[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		atts[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		atts[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		atts[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		atts[1].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		atts[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		VkAttachmentReference mainColorRefs[2]{};
		mainColorRefs[0].attachment = 0;
		mainColorRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		mainColorRefs[1].attachment = 1;
		mainColorRefs[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		VkAttachmentReference overlayColorRef{};
		overlayColorRef.attachment = 0;
		overlayColorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		VkSubpassDescription subs[2]{};
		subs[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subs[0].colorAttachmentCount = 2;
		subs[0].pColorAttachments = mainColorRefs;
		subs[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subs[1].colorAttachmentCount = 1;
		subs[1].pColorAttachments = &overlayColorRef;
		VkSubpassDependency deps[2]{};
		deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		deps[0].dstSubpass = 0;
		deps[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		deps[0].srcAccessMask = 0;
		deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		deps[1].srcSubpass = 0;
		deps[1].dstSubpass = 1;
		deps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		deps[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		deps[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		VkRenderPassCreateInfo rpci{};
		rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rpci.attachmentCount = 2;
		rpci.pAttachments = atts;
		rpci.subpassCount = 2;
		rpci.pSubpasses = subs;
		rpci.dependencyCount = 2;
		rpci.pDependencies = deps;
		if (vkCreateRenderPass(s.device, &rpci, nullptr, &s.mainResumeRenderPass) != VK_SUCCESS) {
			Log::error("Failed to create main resume render pass");
			return;
		}
	}
	VkRenderPassBeginInfo rpBegin{};
	rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBegin.renderPass = s.mainResumeRenderPass;
	rpBegin.framebuffer = s.framebuffers[s.currentImageIndex];
	rpBegin.renderArea.offset.x = 0;
	rpBegin.renderArea.offset.y = 0;
	rpBegin.renderArea.extent = s.swapchainExtent;
	rpBegin.clearValueCount = 0;
	rpBegin.pClearValues = nullptr;
	vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
	statsRenderPass();
	s.mainRenderPassActive = true;
	// Resumed pass starts at subpass 0; allow draws and overlay advance again.
	s.advancedToOverlaySubpass = false;
	// Resume RP is render-pass-compatible with s.renderPass; report the latter
	// so cached pipelines created against the original main RP are reusable.
	s.currentRenderPass = s.renderPass;
	s.currentColorAttachmentCount = 2u;
	s.currentRenderPassHasDepth = false;
}

static bool beginFboRenderPass(VkState &s, VkState::FramebufferHandle &fb, FboBeginClear clearMode) {
	if (!s.frameAcquired || s.device == VK_NULL_HANDLE) {
		return false;
	}
	if (!buildFboResources(s, fb)) {
		return false;
	}
	VkCommandBuffer cmd = s.frameCommandBuffers[s.currentFrame];
	const glm::vec4 &cc = rendererState().pendingClearColor;
	VkClearValue clears[9]{};
	for (uint32_t i = 0; i < fb.colorCount; ++i) {
		clears[i].color.float32[0] = cc.r;
		clears[i].color.float32[1] = cc.g;
		clears[i].color.float32[2] = cc.b;
		clears[i].color.float32[3] = cc.a;
	}
	uint32_t clearCount = fb.colorCount;
	if (fb.depthAttachment != InvalidId) {
		clears[clearCount].depthStencil.depth = 1.0f;
		clears[clearCount].depthStencil.stencil = 0;
		++clearCount;
	}
	bool colorDefined = false;
	bool depthDefined = false;
	if (fb.colorCount > 0u) {
		VkState::TextureHandle *th = findTextureHandle(s, fb.colorAttachments[0]);
		colorDefined = th != nullptr && th->layout != VK_IMAGE_LAYOUT_UNDEFINED;
	}
	if (fb.depthAttachment != InvalidId) {
		VkState::TextureHandle *th = findTextureHandle(s, fb.depthAttachment);
		depthDefined = th != nullptr && th->layout != VK_IMAGE_LAYOUT_UNDEFINED;
	}
	const bool anyDefined = (fb.colorCount > 0u ? colorDefined : depthDefined);

	bool clearColor = false;
	bool clearDepth = false;
	VkRenderPass chosen = fb.renderPass;
	if (clearMode == FboBeginClear::All) {
		clearColor = fb.colorCount > 0u;
		clearDepth = fb.depthAttachment != InvalidId;
		if (anyDefined && fb.renderPassClearDefined != VK_NULL_HANDLE) {
			chosen = fb.renderPassClearDefined;
		} else {
			chosen = fb.renderPass;
		}
	} else if (clearMode == FboBeginClear::Color) {
		clearColor = fb.colorCount > 0u;
		if (colorDefined && fb.renderPassColorClearDefined != VK_NULL_HANDLE) {
			chosen = fb.renderPassColorClearDefined;
		} else if (fb.renderPassColorClear != VK_NULL_HANDLE) {
			chosen = fb.renderPassColorClear;
		} else if (anyDefined && fb.renderPassClearDefined != VK_NULL_HANDLE) {
			chosen = fb.renderPassClearDefined;
			clearDepth = fb.depthAttachment != InvalidId;
		} else {
			chosen = fb.renderPass;
			clearDepth = fb.depthAttachment != InvalidId;
		}
	} else {
		// Auto: preserve defined contents. If only color is undefined, clear
		// color and keep depth (OIT after a depth blit).
		if (!anyDefined && !depthDefined) {
			clearColor = fb.colorCount > 0u;
			clearDepth = fb.depthAttachment != InvalidId;
			chosen = fb.renderPass;
		} else if (!colorDefined && depthDefined && fb.renderPassColorClear != VK_NULL_HANDLE) {
			clearColor = true;
			chosen = fb.renderPassColorClear;
		} else if (fb.renderPassLoad != VK_NULL_HANDLE) {
			chosen = fb.renderPassLoad;
		} else {
			chosen = fb.renderPass;
		}
	}

	VkRenderPassBeginInfo rpBegin{};
	rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBegin.renderPass = chosen;
	rpBegin.framebuffer = fb.framebuffer;
	rpBegin.renderArea.offset.x = 0;
	rpBegin.renderArea.offset.y = 0;
	rpBegin.renderArea.extent.width = fb.width;
	rpBegin.renderArea.extent.height = fb.height;
	if (clearColor || clearDepth) {
		rpBegin.clearValueCount = clearCount;
		rpBegin.pClearValues = clears;
	} else {
		rpBegin.clearValueCount = 0;
		rpBegin.pClearValues = nullptr;
	}
	vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
	statsRenderPass();
	s.fboRenderPassActive = true;
	s.boundFboHandle = fb.id;
	s.currentRenderPass = rpBegin.renderPass;
	s.currentColorAttachmentCount = fb.colorCount;
	s.currentRenderPassHasDepth = (fb.depthAttachment != InvalidId);
	return true;
}

static void destroyFramebufferGpu(VkState &s, VkState::FramebufferHandle &h) {
	queuePendingFboDestroy(s, h);
}

} // namespace _priv

bool readFramebuffer(int x, int y, int w, int h, uint8_t **pixels) {
	_priv::VkState &s = vkstate();
	if (s.device == VK_NULL_HANDLE || s.swapchain == VK_NULL_HANDLE || pixels == nullptr) {
		return false;
	}
	if (!s.frameAcquired) {
		// Without an in-flight frame we have nothing meaningful in the swapchain image.
		Log::error("readFramebuffer called outside of a frame");
		return false;
	}
	if (w <= 0 || h <= 0) {
		return false;
	}
	if (x < 0 || y < 0 || (uint32_t)(x + w) > s.swapchainExtent.width || (uint32_t)(y + h) > s.swapchainExtent.height) {
		Log::error("readFramebuffer region (%d,%d %dx%d) out of swapchain bounds (%ux%u)", x, y, w, h,
				   s.swapchainExtent.width, s.swapchainExtent.height);
		return false;
	}

	const VkDeviceSize bufferSize = (VkDeviceSize)w * (VkDeviceSize)h * 4ull;

	// Host-visible staging buffer to copy the swapchain image into.
	VkBuffer staging = VK_NULL_HANDLE;
	VkDeviceMemory stagingMem = VK_NULL_HANDLE;
	VkBufferCreateInfo bufInfo;
	bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufInfo.pNext = nullptr;
	bufInfo.flags = 0;
	bufInfo.size = bufferSize;
	bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufInfo.queueFamilyIndexCount = 0;
	bufInfo.pQueueFamilyIndices = nullptr;
	if (vkCreateBuffer(s.device, &bufInfo, nullptr, &staging) != VK_SUCCESS) {
		Log::error("readFramebuffer: vkCreateBuffer failed");
		return false;
	}

	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(s.device, staging, &memReq);
	int32_t memType = _priv::findMemoryType(s.physicalDevice, memReq.memoryTypeBits,
											VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
											0);
	if (memType < 0) {
		Log::error("readFramebuffer: no host-visible memory type");
		vkDestroyBuffer(s.device, staging, nullptr);
		return false;
	}
	VkMemoryAllocateInfo allocInfo;
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = (uint32_t)memType;
	if (vkAllocateMemory(s.device, &allocInfo, nullptr, &stagingMem) != VK_SUCCESS) {
		Log::error("readFramebuffer: vkAllocateMemory failed");
		vkDestroyBuffer(s.device, staging, nullptr);
		return false;
	}
	vkBindBufferMemory(s.device, staging, stagingMem, 0);

	VkCommandBuffer cmd = s.frameCommandBuffers[s.currentFrame];

	// End the main render pass, then record image-layout transitions + a copy into the
	// staging buffer, then transition the image to PRESENT_SRC so endFrame() can present it.
	// The render pass has two subpasses; ensure the overlay subpass has been
	// entered before ending so Vulkan does not complain that subpasses were skipped.
	if (!s.advancedToOverlaySubpass) {
		vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
		s.advancedToOverlaySubpass = true;
	}
	vkCmdEndRenderPass(cmd);
	s.mainRenderPassActive = false;

	VkImage img = s.swapchainImages[s.currentImageIndex];

	VkImageMemoryBarrier toTransfer;
	toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	toTransfer.pNext = nullptr;
	toTransfer.srcAccessMask = 0;
	toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	toTransfer.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // render pass final layout
	toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	toTransfer.image = img;
	toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	toTransfer.subresourceRange.baseMipLevel = 0;
	toTransfer.subresourceRange.levelCount = 1;
	toTransfer.subresourceRange.baseArrayLayer = 0;
	toTransfer.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
						 nullptr, 0, nullptr, 1, &toTransfer);

	VkBufferImageCopy region;
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset.x = x;
	region.imageOffset.y = y;
	region.imageOffset.z = 0;
	region.imageExtent.width = (uint32_t)w;
	region.imageExtent.height = (uint32_t)h;
	region.imageExtent.depth = 1;
	vkCmdCopyImageToBuffer(cmd, img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, staging, 1, &region);

	VkImageMemoryBarrier toPresent;
	toPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	toPresent.pNext = nullptr;
	toPresent.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	toPresent.dstAccessMask = 0;
	toPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	toPresent.image = img;
	toPresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	toPresent.subresourceRange.baseMipLevel = 0;
	toPresent.subresourceRange.levelCount = 1;
	toPresent.subresourceRange.baseArrayLayer = 0;
	toPresent.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
						 nullptr, 1, &toPresent);

	if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
		Log::error("readFramebuffer: vkEndCommandBuffer failed");
		vkDestroyBuffer(s.device, staging, nullptr);
		vkFreeMemory(s.device, stagingMem, nullptr);
		return false;
	}

	VkSemaphore waitSemaphore = s.imageAvailableSemaphores[s.currentFrame];
	VkSemaphore signalSemaphore = s.renderFinishedSemaphores[s.currentImageIndex];
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submit;
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &waitSemaphore;
	submit.pWaitDstStageMask = &waitStage;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmd;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &signalSemaphore;

	VkFence fence = s.inFlightFences[s.currentFrame];
	VkResult sr = vkQueueSubmit(s.graphicsQueue, 1, &submit, fence);
	if (sr != VK_SUCCESS) {
		Log::error("readFramebuffer: vkQueueSubmit failed: %s", _priv::vkResultStr(sr));
		vkDestroyBuffer(s.device, staging, nullptr);
		vkFreeMemory(s.device, stagingMem, nullptr);
		s.frameAcquired = false;
		return false;
	}
	s.inFlightFencePending[s.currentFrame] = true;
	vkWaitForFences(s.device, 1, &fence, VK_TRUE, UINT64_MAX);

	// Map staging buffer and copy to caller-owned RGBA buffer.
	void *mapped = nullptr;
	if (vkMapMemory(s.device, stagingMem, 0, bufferSize, 0, &mapped) != VK_SUCCESS) {
		Log::error("readFramebuffer: vkMapMemory failed");
		vkDestroyBuffer(s.device, staging, nullptr);
		vkFreeMemory(s.device, stagingMem, nullptr);
		s.frameAcquired = false;
		return false;
	}

	uint8_t *out = (uint8_t *)core_malloc((size_t)bufferSize);
	if (out == nullptr) {
		vkUnmapMemory(s.device, stagingMem);
		vkDestroyBuffer(s.device, staging, nullptr);
		vkFreeMemory(s.device, stagingMem, nullptr);
		s.frameAcquired = false;
		return false;
	}

	const bool bgra = (s.swapchainFormat == VK_FORMAT_B8G8R8A8_UNORM || s.swapchainFormat == VK_FORMAT_B8G8R8A8_SRGB);
	const uint8_t *src = (const uint8_t *)mapped;
	// Vulkan images store row 0 at the top; the OpenGL backend's readFramebuffer()
	// returns row 0 at the bottom. Flip vertically during the copy so callers
	// (e.g. TestApp::screenShot()'s unconditional flipVerticalRGBA) get a
	// consistent orientation across backends.
	const size_t rowBytes = (size_t)w * 4u;
	for (int row = 0; row < h; ++row) {
		const uint8_t *srcRow = src + (size_t)(h - 1 - row) * rowBytes;
		uint8_t *dstRow = out + (size_t)row * rowBytes;
		if (bgra) {
			// Swizzle BGRA -> RGBA to match the OpenGL backend's readback layout.
			for (int col = 0; col < w; ++col) {
				dstRow[col * 4 + 0] = srcRow[col * 4 + 2];
				dstRow[col * 4 + 1] = srcRow[col * 4 + 1];
				dstRow[col * 4 + 2] = srcRow[col * 4 + 0];
				dstRow[col * 4 + 3] = srcRow[col * 4 + 3];
			}
		} else {
			core_memcpy(dstRow, srcRow, rowBytes);
		}
	}

	vkUnmapMemory(s.device, stagingMem);
	vkDestroyBuffer(s.device, staging, nullptr);
	vkFreeMemory(s.device, stagingMem, nullptr);

	// Present the image that we just copied so the compositor can display the same frame.
	VkPresentInfoKHR present;
	present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present.pNext = nullptr;
	present.waitSemaphoreCount = 1;
	present.pWaitSemaphores = &signalSemaphore;
	present.swapchainCount = 1;
	present.pSwapchains = &s.swapchain;
	present.pImageIndices = &s.currentImageIndex;
	present.pResults = nullptr;
	VkResult pr = vkQueuePresentKHR(s.presentQueue, &present);
	if (pr == VK_ERROR_OUT_OF_DATE_KHR || pr == VK_SUBOPTIMAL_KHR) {
		_priv::destroySwapchainResources(s, false);
		if (_priv::createSwapchain(s)) {
			_priv::createFramebuffers(s.device, s.renderPass, s.swapchainImageViews, s.glowImageViews,
									  s.swapchainImageCount, s.swapchainExtent, s.framebuffers);
		}
	} else if (pr != VK_SUCCESS) {
		Log::error("readFramebuffer: vkQueuePresentKHR failed: %s", _priv::vkResultStr(pr));
	}

	s.currentFrame = (s.currentFrame + 1u) % _priv::MAX_FRAMES_IN_FLIGHT;
	s.frameAcquired = false;

	*pixels = out;
	return true;
}

bool bindVertexArray(Id handle) {
	if (rendererState().vertexArrayHandle == handle) {
		return true;
	}
	rendererState().vertexArrayHandle = handle;
	return true;
}

bool bindBuffer(BufferType type, Id handle) {
	rendererState().bufferHandle[core::enumVal(type)] = handle;
	// Mirror GL VAO semantics: while a VAO is bound, an IndexBuffer binding
	// is associated with that VAO so subsequent draws know which index buffer
	// to use.
	if (type == BufferType::IndexBuffer) {
		_priv::VkState &s = vkstate();
		_priv::VkState::VaoHandle *vao = _priv::findVaoHandle(s, rendererState().vertexArrayHandle);
		if (vao != nullptr) {
			vao->indexBuffer = handle;
		}
	}
	return true;
}

bool unbindBuffer(BufferType type) {
	return bindBuffer(type, InvalidId);
}

bool bindBufferBase(BufferType type, Id handle, uint32_t index) {
	const uint64_t key = (static_cast<uint64_t>(core::enumVal(type)) << 32) | index;
	rendererState().bufferBaseBindings.put(key, handle);
	return true;
}

void genBuffers(uint8_t amount, Id *ids) {
	statsResourceCreate(amount);
	_priv::VkState &s = vkstate();
	for (uint8_t i = 0; i < amount; ++i) {
		if (!_priv::ensureBufferCapacity(s)) {
			ids[i] = InvalidId;
			continue;
		}
		const uint32_t index = s.bufferHandleCount++;
		_priv::VkState::BufferHandle &h = s.bufferHandles[index];
		h.id = _priv::allocHandleId(s);
		h.alive = true;
		h.type = BufferType::Max;
		h.size = 0u;
		h.data = nullptr;
		if (_priv::ensureIdIndexCapacity(&s.bufferIdIndex, &s.bufferIdIndexCapacity, h.id)) {
			s.bufferIdIndex[h.id] = index;
		}
		ids[i] = h.id;
	}
}

void deleteBuffers(uint8_t amount, Id *ids) {
	if (amount == 0) {
		return;
	}
	statsResourceDestroy(amount);
	_priv::VkState &s = vkstate();
	for (uint8_t i = 0; i < amount; ++i) {
		Id &id = ids[i];
		if (id == InvalidId) {
			continue;
		}
		_priv::VkState::BufferHandle *h = _priv::findBufferHandle(s, id);
		if (h != nullptr) {
			_priv::destroyBufferAlloc(s, *h);
			h->alive = false;
			h->type = BufferType::Max;
			h->size = 0u;
			if (h->data != nullptr) {
				core_free((void *)h->data);
				h->data = nullptr;
			}
			if (h->stableData != nullptr) {
				core_free(h->stableData);
				h->stableData = nullptr;
				h->stableDataSize = 0u;
			}
			h->pushOverlayFieldCount = 0u;
			h->pushConstantSize = 0u;
			h->stableRingFrame = ~0u;
			h->stableRingBuffer = VK_NULL_HANDLE;
		}
		if ((uint32_t)id < s.bufferIdIndexCapacity) {
			s.bufferIdIndex[id] = ~0u;
		}
		id = InvalidId;
	}
}

void genVertexArrays(uint8_t amount, Id *ids) {
	statsResourceCreate(amount);
	_priv::VkState &s = vkstate();
	for (uint8_t i = 0; i < amount; ++i) {
		if (!_priv::ensureVaoCapacity(s)) {
			ids[i] = InvalidId;
			continue;
		}
		const uint32_t index = s.vaoHandleCount++;
		_priv::VkState::VaoHandle &v = s.vaoHandles[index];
		v.id = _priv::allocHandleId(s);
		v.alive = true;
		v.attributeCount = 0u;
		v.indexBuffer = InvalidId;
		for (int j = 0; j < 16; ++j) {
			v.arrayBuffers[j] = InvalidId;
		}
		if (_priv::ensureIdIndexCapacity(&s.vaoIdIndex, &s.vaoIdIndexCapacity, v.id)) {
			s.vaoIdIndex[v.id] = index;
		}
		ids[i] = v.id;
	}
}

void deleteShader(Id &id) {
	if (id == InvalidId) {
		return;
	}
	_priv::VkState &s = vkstate();
	_priv::VkState::ShaderHandle *h = _priv::findShaderHandle(s, id);
	if (h != nullptr) {
		if (h->module != VK_NULL_HANDLE && s.device != VK_NULL_HANDLE) {
			vkDestroyShaderModule(s.device, h->module, nullptr);
			h->module = VK_NULL_HANDLE;
		}
		h->source.clear();
		h->alive = false;
	}
	id = InvalidId;
}

Id genShader(ShaderType type) {
	_priv::VkState &s = vkstate();
	if (!_priv::ensureShaderCapacity(s)) {
		Log::error("Failed to allocate shader handle registry");
		return InvalidId;
	}
	_priv::VkState::ShaderHandle &h = s.shaderHandles[s.shaderHandleCount++];
	h.id = _priv::allocHandleId(s);
	h.type = type;
	h.alive = true;
	return h.id;
}

void deleteProgram(Id &id) {
	if (id == InvalidId) {
		return;
	}
	_priv::VkState &s = vkstate();
	_priv::VkState::ProgramHandle *h = _priv::findProgramHandle(s, id);
	if (h != nullptr) {
		if (s.device != VK_NULL_HANDLE) {
			vkDeviceWaitIdle(s.device);
			for (uint32_t i = 0; i < _priv::MAX_FRAMES_IN_FLIGHT; ++i) {
				if (s.frameCommandPools[i] != VK_NULL_HANDLE) {
					vkResetCommandPool(s.device, s.frameCommandPools[i], 0);
				}
			}
			s.frameAcquired = false;
			s.mainRenderPassActive = false;
			s.fboRenderPassActive = false;
		}
		if (h->computePipeline != VK_NULL_HANDLE && s.device != VK_NULL_HANDLE) {
			vkDestroyPipeline(s.device, h->computePipeline, nullptr);
			h->computePipeline = VK_NULL_HANDLE;
		}
		h->computeShader = InvalidId;
		for (uint32_t j = 0; j < h->pipelineCount; ++j) {
			if (h->pipelines != nullptr && h->pipelines[j].pipeline != VK_NULL_HANDLE && s.device != VK_NULL_HANDLE) {
				vkDestroyPipeline(s.device, h->pipelines[j].pipeline, nullptr);
			}
		}
		delete[] h->pipelines;
		h->pipelines = nullptr;
		h->pipelineCount = 0u;
		h->pipelineCapacity = 0u;
		if (h->layout != VK_NULL_HANDLE && s.device != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(s.device, h->layout, nullptr);
			h->layout = VK_NULL_HANDLE;
		}
		if (h->descriptorSetLayout != VK_NULL_HANDLE && s.device != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(s.device, h->descriptorSetLayout, nullptr);
			h->descriptorSetLayout = VK_NULL_HANDLE;
		}
		h->name.clear();
		h->uniformOffsets.clear();
		h->uniforms.clear();
		h->attributes.clear();
		h->registeredBindingCount = 0u;
		h->pushConstantSize = 0u;
		h->alive = false;
	}
	id = InvalidId;
}

Id genProgram() {
	_priv::VkState &s = vkstate();
	if (!_priv::ensureProgramCapacity(s)) {
		Log::error("Failed to allocate program handle registry");
		return InvalidId;
	}
	_priv::VkState::ProgramHandle &h = s.programHandles[s.programHandleCount++];
	h.id = _priv::allocHandleId(s);
	h.alive = true;
	return h.id;
}

void deleteVertexArrays(uint8_t amount, Id *ids) {
	if (amount == 0) {
		return;
	}
	statsResourceDestroy(amount);
	_priv::VkState &s = vkstate();
	for (uint8_t i = 0; i < amount; ++i) {
		Id &id = ids[i];
		if (id == InvalidId) {
			continue;
		}
		_priv::VkState::VaoHandle *v = _priv::findVaoHandle(s, id);
		if (v != nullptr) {
			v->alive = false;
			v->attributeCount = 0u;
			v->indexBuffer = InvalidId;
		}
		if ((uint32_t)id < s.vaoIdIndexCapacity) {
			s.vaoIdIndex[id] = ~0u;
		}
		id = InvalidId;
	}
}

void genTextures(const TextureConfig &cfg, uint8_t amount, Id *ids) {
	statsResourceCreate(amount);
	_priv::VkState &s = vkstate();
	for (uint8_t i = 0; i < amount; ++i) {
		if (!_priv::ensureTextureCapacity(s)) {
			ids[i] = InvalidId;
			continue;
		}
		_priv::VkState::TextureHandle &h = s.textureHandles[s.textureHandleCount++];
		h = _priv::VkState::TextureHandle{};
		h.id = _priv::allocHandleId(s);
		h.alive = true;
		h.type = cfg.type();
		h.format = cfg.format();
		h.layers = cfg.layers();
		h.isDepth = _priv::textureFormatIsDepth(cfg.format());
		ids[i] = h.id;
	}
}

static void destroyTextureGpu(_priv::VkState &s, _priv::VkState::TextureHandle &h) {
	if (h.imguiSet != VK_NULL_HANDLE && s.descriptorPool != VK_NULL_HANDLE) {
		vkFreeDescriptorSets(s.device, s.descriptorPool, 1, &h.imguiSet);
		h.imguiSet = VK_NULL_HANDLE;
	}
	if (h.sampler != VK_NULL_HANDLE) {
		vkDestroySampler(s.device, h.sampler, nullptr);
		h.sampler = VK_NULL_HANDLE;
	}
	if (h.view != VK_NULL_HANDLE) {
		vkDestroyImageView(s.device, h.view, nullptr);
		h.view = VK_NULL_HANDLE;
	}
	if (h.image != VK_NULL_HANDLE) {
		vkDestroyImage(s.device, h.image, nullptr);
		h.image = VK_NULL_HANDLE;
	}
	if (h.memory != VK_NULL_HANDLE && !h.slabMemory) {
		vkFreeMemory(s.device, h.memory, nullptr);
	}
	h.memory = VK_NULL_HANDLE;
	h.slabMemory = false;
	h.storageBit = false;
	h.hasContent = false;
	h.layout = VK_IMAGE_LAYOUT_UNDEFINED;
}

void deleteTextures(uint8_t amount, Id *ids) {
	if (amount == 0) {
		return;
	}
	statsResourceDestroy(amount);
	_priv::VkState &s = vkstate();
	if (s.device != VK_NULL_HANDLE) {
		_priv::waitAllInFlightFrames(s);
	}
	for (uint8_t i = 0; i < amount; ++i) {
		if (ids[i] == InvalidId) {
			continue;
		}
		_priv::VkState::TextureHandle *h = _priv::findTextureHandle(s, ids[i]);
		if (h != nullptr) {
			destroyTextureGpu(s, *h);
			h->alive = false;
		}
		ids[i] = InvalidId;
	}
}

void genFramebuffers(uint8_t amount, Id *ids) {
	statsResourceCreate(amount);
	_priv::VkState &s = vkstate();
	for (uint8_t i = 0; i < amount; ++i) {
		if (!_priv::ensureFramebufferCapacity(s)) {
			ids[i] = InvalidId;
			continue;
		}
		_priv::VkState::FramebufferHandle &h = s.framebufferHandles[s.framebufferHandleCount++];
		h = _priv::VkState::FramebufferHandle{};
		h.id = _priv::allocHandleId(s);
		h.alive = true;
		ids[i] = h.id;
	}
}

void deleteFramebuffers(uint8_t amount, Id *ids) {
	if (amount == 0) {
		return;
	}
	statsResourceDestroy(amount);
	_priv::VkState &s = vkstate();
	for (uint8_t i = 0; i < amount; ++i) {
		if (ids[i] == InvalidId) {
			continue;
		}
		_priv::VkState::FramebufferHandle *h = _priv::findFramebufferHandle(s, ids[i]);
		if (h != nullptr) {
			if (s.frameAcquired && s.fboRenderPassActive && s.boundFboHandle == h->id) {
				_priv::endFboRenderPassIfActive(s);
			}
			_priv::destroyFramebufferGpu(s, *h);
			if (s.graphicsQueue != VK_NULL_HANDLE) {
				vkQueueWaitIdle(s.graphicsQueue);
			}
			_priv::releasePendingFboDestroys(s);
			_priv::releasePendingBufferDestroys(s);
			h->alive = false;
		}
		ids[i] = InvalidId;
	}
}

void genRenderbuffers(uint8_t amount, Id *ids) {
	// Allocate as texture handles so FBO depth attachments can use findTextureHandle().
	statsResourceCreate(amount);
	_priv::VkState &s = vkstate();
	for (uint8_t i = 0; i < amount; ++i) {
		if (!_priv::ensureTextureCapacity(s)) {
			ids[i] = InvalidId;
			continue;
		}
		_priv::VkState::TextureHandle &h = s.textureHandles[s.textureHandleCount++];
		h = _priv::VkState::TextureHandle{};
		h.id = _priv::allocHandleId(s);
		h.alive = true;
		h.isDepth = true;
		ids[i] = h.id;
	}
}

void deleteRenderbuffers(uint8_t amount, Id *ids) {
	deleteTextures(amount, ids);
}


void configureAttribute(const Attribute &a) {
	_priv::VkState &s = vkstate();
	_priv::VkState::VaoHandle *vao = _priv::findVaoHandle(s, rendererState().vertexArrayHandle);
	if (vao == nullptr) {
		return;
	}
	// Replace an attribute with matching location if it already exists.
	for (uint32_t i = 0; i < vao->attributeCount; ++i) {
		if (vao->attributes[i].location == a.location) {
			vao->attributes[i] = a;
			if (a.bufferIndex >= 0 && a.bufferIndex < 16) {
				vao->arrayBuffers[a.bufferIndex] = rendererState().bufferHandle[core::enumVal(BufferType::ArrayBuffer)];
			}
			return;
		}
	}
	if (vao->attributeCount >= 16u) {
		Log::warn("configureAttribute: too many attributes captured in VAO %u", vao->id);
		return;
	}
	vao->attributes[vao->attributeCount++] = a;
	if (a.bufferIndex >= 0 && a.bufferIndex < 16) {
		vao->arrayBuffers[a.bufferIndex] = rendererState().bufferHandle[core::enumVal(BufferType::ArrayBuffer)];
	}
}

Id bindFramebuffer(Id handle, FrameBufferMode mode) {
	_priv::VkState &s = vkstate();
	s.drawBufferMask = 0xFFu;
	const Id previous = s.currentFramebuffer;
	rendererState().framebufferHandle = handle;
	rendererState().framebufferMode = mode;
	if (handle == s.currentFramebuffer) {
		// Binding may have been recorded outside a frame (no render pass started yet).
		if (s.frameAcquired) {
			if (handle != InvalidId && !s.fboRenderPassActive) {
				_priv::VkState::FramebufferHandle *fb = _priv::findFramebufferHandle(s, handle);
				if (fb != nullptr) {
					_priv::endFboRenderPassIfActive(s);
					_priv::endMainRenderPassIfActive(s);
					_priv::beginFboRenderPass(s, *fb);
				}
			} else if (handle == InvalidId && !s.mainRenderPassActive) {
				_priv::endFboRenderPassIfActive(s);
				_priv::resumeMainRenderPass(s);
			}
		}
		return previous;
	}
	s.currentFramebuffer = handle;
	if (!s.frameAcquired) {
		// Outside a frame we just track the binding; the next frame will use
		// it on first draw.
		return previous;
	}
	if (handle == InvalidId) {
		// Returning to default (swapchain) framebuffer.
		_priv::endFboRenderPassIfActive(s);
		_priv::resumeMainRenderPass(s);
	} else {
		_priv::VkState::FramebufferHandle *fb = _priv::findFramebufferHandle(s, handle);
		if (fb == nullptr) {
			Log::warn("bindFramebuffer: unknown handle %u", handle);
			return previous;
		}
		_priv::endFboRenderPassIfActive(s);
		_priv::endMainRenderPassIfActive(s);
		_priv::beginFboRenderPass(s, *fb);
	}
	return previous;
}

static void cmdBlitImageBarrier(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
								VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage,
								VkPipelineStageFlags dstStage, VkImageAspectFlags aspect) {
	VkImageMemoryBarrier bar;
	bar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	bar.pNext = nullptr;
	bar.srcAccessMask = srcAccess;
	bar.dstAccessMask = dstAccess;
	bar.oldLayout = oldLayout;
	bar.newLayout = newLayout;
	bar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bar.image = image;
	bar.subresourceRange.aspectMask = aspect;
	bar.subresourceRange.baseMipLevel = 0;
	bar.subresourceRange.levelCount = 1;
	bar.subresourceRange.baseArrayLayer = 0;
	bar.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &bar);
}

void blitFramebuffer(Id handle, Id target, ClearFlag flag, int width, int height) {
	_priv::VkState &s = vkstate();
	if (s.device == VK_NULL_HANDLE || !s.frameAcquired || width <= 0 || height <= 0) {
		return;
	}
	statsBlit();
	_priv::VkState::FramebufferHandle *srcFb = _priv::findFramebufferHandle(s, handle);
	if (srcFb == nullptr) {
		Log::error("blitFramebuffer: unknown source framebuffer %u", handle);
		return;
	}

	const bool blitColor = (flag & ClearFlag::Color) != ClearFlag::None;
	const bool blitDepth = (flag & ClearFlag::Depth) != ClearFlag::None;
	if (!blitColor && !blitDepth) {
		return;
	}

	_priv::endFboRenderPassIfActive(s);
	const bool resumeMain = (target == InvalidId);
	if (resumeMain) {
		_priv::endMainRenderPassIfActive(s);
	}

	VkCommandBuffer cmd = s.frameCommandBuffers[s.currentFrame];
	const int blitW = width;
	const int blitH = height;

	if (blitColor && srcFb->colorCount > 0u) {
		_priv::VkState::TextureHandle *srcTex = _priv::findTextureHandle(s, srcFb->colorAttachments[0]);
		if (srcTex == nullptr || srcTex->image == VK_NULL_HANDLE) {
			Log::error("blitFramebuffer: source color attachment has no image");
			if (resumeMain) {
				_priv::resumeMainRenderPass(s);
			}
			return;
		}
		VkImage dstImage = VK_NULL_HANDLE;
		VkImageLayout dstOldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		_priv::VkState::TextureHandle *dstTex = nullptr;
		int dstW = blitW;
		int dstH = blitH;
		if (target == InvalidId) {
			dstImage = s.swapchainImages[s.currentImageIndex];
			dstOldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			dstW = (int)s.swapchainExtent.width;
			dstH = (int)s.swapchainExtent.height;
		} else {
			_priv::VkState::FramebufferHandle *dstFb = _priv::findFramebufferHandle(s, target);
			if (dstFb == nullptr || dstFb->colorCount == 0u) {
				Log::error("blitFramebuffer: unknown target framebuffer %u", target);
				if (resumeMain) {
					_priv::resumeMainRenderPass(s);
				}
				return;
			}
			dstTex = _priv::findTextureHandle(s, dstFb->colorAttachments[0]);
			if (dstTex == nullptr || dstTex->image == VK_NULL_HANDLE) {
				Log::error("blitFramebuffer: target color attachment has no image");
				if (resumeMain) {
					_priv::resumeMainRenderPass(s);
				}
				return;
			}
			dstImage = dstTex->image;
			dstOldLayout = dstTex->layout;
			dstW = dstTex->width;
			dstH = dstTex->height;
		}

		const VkImageLayout srcOldLayout = srcTex->layout != VK_IMAGE_LAYOUT_UNDEFINED
											   ? srcTex->layout
											   : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		cmdBlitImageBarrier(cmd, srcTex->image, srcOldLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
							VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
							VK_IMAGE_ASPECT_COLOR_BIT);
		const VkAccessFlags dstSrcAccess =
			dstOldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? 0 : VK_ACCESS_SHADER_READ_BIT;
		cmdBlitImageBarrier(cmd, dstImage, dstOldLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstSrcAccess,
							VK_ACCESS_TRANSFER_WRITE_BIT,
							VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
							VK_PIPELINE_STAGE_TRANSFER_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

		VkImageBlit region;
		core_memset(&region, 0, sizeof(region));
		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.layerCount = 1;
		region.srcOffsets[1].x = core_min(blitW, srcTex->width);
		region.srcOffsets[1].y = core_min(blitH, srcTex->height);
		region.srcOffsets[1].z = 1;
		region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.dstSubresource.layerCount = 1;
		region.dstOffsets[1].x = core_min(blitW, dstW);
		region.dstOffsets[1].y = core_min(blitH, dstH);
		region.dstOffsets[1].z = 1;
		vkCmdBlitImage(cmd, srcTex->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage,
					   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR);

		cmdBlitImageBarrier(cmd, srcTex->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT,
							VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
							VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
		srcTex->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		if (target == InvalidId) {
			cmdBlitImageBarrier(cmd, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
								VK_ACCESS_TRANSFER_WRITE_BIT, 0, VK_PIPELINE_STAGE_TRANSFER_BIT,
								VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
		} else if (dstTex != nullptr) {
			cmdBlitImageBarrier(cmd, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
								VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
								VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
			dstTex->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
	}

	if (blitDepth && target != InvalidId) {
		_priv::VkState::FramebufferHandle *dstFb = _priv::findFramebufferHandle(s, target);
		if (srcFb->depthAttachment == InvalidId || dstFb == nullptr || dstFb->depthAttachment == InvalidId) {
			Log::error("blitFramebuffer: depth blit requires depth attachments on source and target");
		} else {
			_priv::VkState::TextureHandle *srcDepth = _priv::findTextureHandle(s, srcFb->depthAttachment);
			_priv::VkState::TextureHandle *dstDepth = _priv::findTextureHandle(s, dstFb->depthAttachment);
			if (srcDepth == nullptr || srcDepth->image == VK_NULL_HANDLE || dstDepth == nullptr ||
				dstDepth->image == VK_NULL_HANDLE) {
				Log::error("blitFramebuffer: depth attachment has no image");
			} else if (srcDepth->format != dstDepth->format) {
				Log::error("blitFramebuffer: depth format mismatch (%d vs %d)", (int)srcDepth->format,
						   (int)dstDepth->format);
			} else {
				const VkImageAspectFlags aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
				const VkImageLayout srcOldLayout = srcDepth->layout != VK_IMAGE_LAYOUT_UNDEFINED
													   ? srcDepth->layout
													   : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				const VkImageLayout dstOldLayout = dstDepth->layout;
				const VkPipelineStageFlags depthStages =
					VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				const VkAccessFlags depthReadAccess =
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
				cmdBlitImageBarrier(cmd, srcDepth->image, srcOldLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
									depthReadAccess, VK_ACCESS_TRANSFER_READ_BIT, depthStages,
									VK_PIPELINE_STAGE_TRANSFER_BIT, aspect);
				cmdBlitImageBarrier(cmd, dstDepth->image, dstOldLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
									dstOldLayout == VK_IMAGE_LAYOUT_UNDEFINED ? 0 : depthReadAccess,
									VK_ACCESS_TRANSFER_WRITE_BIT, depthStages, VK_PIPELINE_STAGE_TRANSFER_BIT, aspect);

				VkImageBlit region;
				core_memset(&region, 0, sizeof(region));
				region.srcSubresource.aspectMask = aspect;
				region.srcSubresource.layerCount = 1;
				region.srcOffsets[1].x = core_min(blitW, srcDepth->width);
				region.srcOffsets[1].y = core_min(blitH, srcDepth->height);
				region.srcOffsets[1].z = 1;
				region.dstSubresource.aspectMask = aspect;
				region.dstSubresource.layerCount = 1;
				region.dstOffsets[1].x = core_min(blitW, dstDepth->width);
				region.dstOffsets[1].y = core_min(blitH, dstDepth->height);
				region.dstOffsets[1].z = 1;
				vkCmdBlitImage(cmd, srcDepth->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstDepth->image,
							   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_NEAREST);

				cmdBlitImageBarrier(cmd, srcDepth->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
									VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT,
									depthReadAccess, VK_PIPELINE_STAGE_TRANSFER_BIT, depthStages, aspect);
				srcDepth->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				cmdBlitImageBarrier(cmd, dstDepth->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
									VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
									depthReadAccess, VK_PIPELINE_STAGE_TRANSFER_BIT, depthStages, aspect);
				dstDepth->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
		}
	}

	if (resumeMain) {
		_priv::resumeMainRenderPass(s);
	}
}

void blitFramebuffer(Id handle, Id target, FrameBufferAttachment attachment, int width, int height) {
	(void)handle;
	(void)target;
	(void)attachment;
	(void)width;
	(void)height;
}

void drawBuffers(uint8_t n, const FrameBufferAttachment *attachments) {
	_priv::VkState &s = vkstate();
	uint8_t mask = 0u;
	if (attachments != nullptr) {
		const int color0 = core::enumVal(FrameBufferAttachment::Color0);
		for (uint8_t i = 0; i < n; ++i) {
			const int idx = core::enumVal(attachments[i]) - color0;
			if (idx >= 0 && idx < 8) {
				mask |= (uint8_t)(1u << idx);
			}
		}
	}
	s.drawBufferMask = mask;
}

bool setupRenderBuffer(Id rbo, TextureFormat format, int w, int h, int samples) {
	_priv::VkState &s = vkstate();
	_priv::VkState::TextureHandle *th = _priv::findTextureHandle(s, rbo);
	if (th == nullptr || w <= 0 || h <= 0) {
		return false;
	}
	(void)samples;
	th->isDepth = _priv::textureFormatIsDepth(format);
	th->format = format;
	th->type = TextureType::Texture2D;
	th->width = w;
	th->height = h;
	TextureConfig cfg;
	cfg.format(format);
	cfg.type(TextureType::Texture2D);
	uploadTexture(rbo, w, h, nullptr, 0, cfg);
	if (th->image != VK_NULL_HANDLE) {
		return true;
	}
	if (format != TextureFormat::D32F) {
		th->format = TextureFormat::D32F;
		th->isDepth = true;
		cfg.format(TextureFormat::D32F);
		uploadTexture(rbo, w, h, nullptr, 0, cfg);
	}
	return th->image != VK_NULL_HANDLE;
}

Id bindRenderbuffer(Id handle) {
	return InvalidId;
}

void bufferData(Id handle, BufferType type, BufferMode mode, const void *data, size_t size) {
	if (size <= 0u) {
		return;
	}
	statsUploadScopeBegin();
	_priv::VkState &s = vkstate();
	_priv::VkState::BufferHandle *h = _priv::findBufferHandle(s, handle);
	if (h == nullptr) {
		statsUploadScopeEnd();
		return;
	}
	h->type = type;
	if (h->size != size) {
		if (h->data != nullptr) {
			core_free((void *)h->data);
			h->data = nullptr;
		}
		h->data = (uint8_t *)core_malloc(size);
		h->size = size;
		if (h->stableData != nullptr) {
			core_free(h->stableData);
			h->stableData = nullptr;
			h->stableDataSize = 0u;
		}
	}
	h->partialDirty = false;
	h->stableRingFrame = ~0u;
	h->stableRingBuffer = VK_NULL_HANDLE;
	if (data != nullptr && h->data != nullptr) {
		core_memcpy(h->data, data, size);
	}
	// Upload to GPU buffer (DEVICE_LOCAL via staging, or host-mapped uniforms).
	if (_priv::ensureBufferAlloc(s, *h, size)) {
		if (data != nullptr) {
			if (!_priv::uploadBufferRegion(s, *h, 0u, data, size)) {
				Log::error("bufferData: upload failed (size=%zu)", size);
			} else {
				statsBufferUpdate(size);
			}
		}
	}
	statsUploadScopeEnd();
}

void bufferSubData(Id handle, BufferType type, intptr_t offset, const void *data, size_t size) {
	if (size == 0u) {
		return;
	}
	statsUploadScopeBegin();
	_priv::VkState &s = vkstate();
	_priv::VkState::BufferHandle *h = _priv::findBufferHandle(s, handle);
	if (h == nullptr || data == nullptr) {
		statsUploadScopeEnd();
		return;
	}
	if (offset < 0 || (size_t)offset + size > h->size) {
		statsUploadScopeEnd();
		return;
	}
	if (h->data != nullptr) {
		core_memcpy(h->data + offset, data, size);
	}
	if (h->buffer != VK_NULL_HANDLE) {
		if (!_priv::uploadBufferRegion(s, *h, (size_t)offset, data, size)) {
			Log::error("bufferSubData: upload failed (offset=%zu size=%zu)", (size_t)offset, size);
			statsUploadScopeEnd();
			return;
		}
	}
	h->partialDirty = true;
	h->partialOffset = (size_t)offset;
	h->partialSize = size;
	statsBufferUpdate(size);
	statsUploadScopeEnd();
}

bool bufferStorage(Id handle, BufferType type, size_t size, BufferStorageFlag flags) {
	(void)flags;
	if (handle == InvalidId || size == 0u) {
		return false;
	}
	_priv::VkState &s = vkstate();
	_priv::VkState::BufferHandle *h = _priv::findBufferHandle(s, handle);
	if (h == nullptr) {
		return false;
	}
	h->type = type;
	if (h->size < size) {
		if (h->data != nullptr) {
			core_free((void *)h->data);
			h->data = nullptr;
		}
		h->data = (uint8_t *)core_malloc(size);
		if (h->data == nullptr) {
			return false;
		}
		core_memset(h->data, 0, size);
		h->size = size;
	}
	return _priv::ensureBufferAlloc(s, *h, size);
}

bool copyBufferSubData(Id readBuffer, intptr_t readOffset, Id writeBuffer, intptr_t writeOffset, size_t size) {
	if (readBuffer == InvalidId || writeBuffer == InvalidId || size == 0u) {
		return false;
	}
	statsUploadScopeBegin();
	_priv::VkState &s = vkstate();
	if (!s.frameAcquired) {
		statsUploadScopeEnd();
		return false;
	}
	_priv::VkState::BufferHandle *src = _priv::findBufferHandle(s, readBuffer);
	_priv::VkState::BufferHandle *dst = _priv::findBufferHandle(s, writeBuffer);
	if (src == nullptr || dst == nullptr || src->buffer == VK_NULL_HANDLE || dst->buffer == VK_NULL_HANDLE) {
		statsUploadScopeEnd();
		return false;
	}
	if (readOffset < 0 || writeOffset < 0) {
		statsUploadScopeEnd();
		return false;
	}
	if ((size_t)readOffset + size > src->size || (size_t)writeOffset + size > dst->size) {
		statsUploadScopeEnd();
		return false;
	}
	// Host fallback: both buffers are persistently mapped.
	if (src->mapped != nullptr && dst->mapped != nullptr) {
		core_memcpy((uint8_t *)dst->mapped + writeOffset, (const uint8_t *)src->mapped + readOffset, size);
		if (dst->data != nullptr) {
			core_memcpy(dst->data + writeOffset, (const uint8_t *)src->mapped + readOffset, size);
		}
		statsBufferUpdate(size);
		statsUploadScopeEnd();
		return true;
	}
	// DEVICE_LOCAL path: GPU copy into the current frame command buffer.
	if (src->buffer == VK_NULL_HANDLE || dst->buffer == VK_NULL_HANDLE) {
		statsUploadScopeEnd();
		return false;
	}
	_priv::endFboRenderPassIfActive(s);
	_priv::endMainRenderPassIfActive(s);
	VkCommandBuffer cmd = s.frameCommandBuffers[s.currentFrame];
	VkBufferCopy region{};
	region.srcOffset = (VkDeviceSize)readOffset;
	region.dstOffset = (VkDeviceSize)writeOffset;
	region.size = (VkDeviceSize)size;
	VkBufferMemoryBarrier barriers[2]{};
	barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barriers[0].srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT |
								 VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
	barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barriers[0].buffer = src->buffer;
	barriers[0].offset = (VkDeviceSize)readOffset;
	barriers[0].size = (VkDeviceSize)size;
	barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barriers[1].srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT |
								 VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
	barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barriers[1].buffer = dst->buffer;
	barriers[1].offset = (VkDeviceSize)writeOffset;
	barriers[1].size = (VkDeviceSize)size;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
								  VK_PIPELINE_STAGE_TRANSFER_BIT,
						 VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 2, barriers, 0, nullptr);
	vkCmdCopyBuffer(cmd, src->buffer, dst->buffer, 1, &region);
	VkBufferMemoryBarrier after{};
	after.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	after.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	after.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
	after.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	after.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	after.buffer = dst->buffer;
	after.offset = (VkDeviceSize)writeOffset;
	after.size = (VkDeviceSize)size;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
						 VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 1,
						 &after, 0, nullptr);
	if (dst->data != nullptr && src->data != nullptr) {
		core_memcpy(dst->data + writeOffset, src->data + readOffset, size);
	}
	statsBufferUpdate(size);
	statsUploadScopeEnd();
	return true;
}

const glm::vec4 &framebufferUV() {
	static glm::vec4 uv(0.0f, 0.0f, 1.0f, 1.0f);
	return uv;
}

bool bindFrameBufferAttachment(Id fbo, Id texture, FrameBufferAttachment attachment, int layerIndex, bool clear) {
	_priv::VkState &s = vkstate();
	if (fbo == InvalidId || texture == InvalidId) {
		return false;
	}
	_priv::VkState::FramebufferHandle *fb = _priv::findFramebufferHandle(s, fbo);
	if (fb == nullptr) {
		return false;
	}
	const int aIdx = core::enumVal(attachment);
	const int color0 = core::enumVal(FrameBufferAttachment::Color0);
	const bool isDepthAtt =
		attachment == FrameBufferAttachment::Depth || attachment == FrameBufferAttachment::DepthStencil;
	const bool wasBound = s.fboRenderPassActive && s.boundFboHandle == fbo;
	Id oldTex = InvalidId;
	int oldLayer = fb->depthLayer;
	if (isDepthAtt) {
		oldTex = fb->depthAttachment;
		fb->depthAttachment = texture;
		fb->depthLayer = layerIndex;
	} else if (aIdx >= color0) {
		const uint32_t slot = (uint32_t)(aIdx - color0);
		if (slot >= lengthof(fb->colorAttachments)) {
			return false;
		}
		oldTex = fb->colorAttachments[slot];
		fb->colorAttachments[slot] = texture;
		if (slot + 1u > fb->colorCount) {
			fb->colorCount = slot + 1u;
		}
	} else {
		// Stencil-only attachment not handled separately for now.
		return false;
	}

	const bool texChanged = oldTex != texture;
	const bool layerChanged = isDepthAtt && oldLayer != layerIndex;
	if (wasBound) {
		_priv::endFboRenderPassIfActive(s);
	}

	if (layerChanged && !texChanged && fb->framebuffer != VK_NULL_HANDLE && fb->renderPass != VK_NULL_HANDLE &&
		!fb->dirty) {
		_priv::VkState::TextureHandle *th = _priv::findTextureHandle(s, fb->depthAttachment);
		if (th == nullptr || th->view == VK_NULL_HANDLE) {
			return false;
		}
		_priv::queuePendingFramebufferAndView(s, fb->framebuffer, fb->depthLayerView);
		fb->framebuffer = VK_NULL_HANDLE;
		fb->depthLayerView = VK_NULL_HANDLE;
		VkImageView depthView = VK_NULL_HANDLE;
		if (!_priv::createDepthLayerView(s, *fb, *th, &depthView)) {
			return false;
		}
		VkImageView views[9];
		uint32_t viewCount = 0;
		for (uint32_t i = 0; i < fb->colorCount; ++i) {
			_priv::VkState::TextureHandle *cth = _priv::findTextureHandle(s, fb->colorAttachments[i]);
			if (cth == nullptr || cth->view == VK_NULL_HANDLE) {
				return false;
			}
			views[viewCount++] = cth->view;
		}
		views[viewCount++] = depthView;
		VkFramebufferCreateInfo fci{};
		fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fci.renderPass = fb->renderPass;
		fci.attachmentCount = viewCount;
		fci.pAttachments = views;
		fci.width = fb->width;
		fci.height = fb->height;
		fci.layers = 1;
		if (vkCreateFramebuffer(s.device, &fci, nullptr, &fb->framebuffer) != VK_SUCCESS) {
			Log::error("FBO %u: vkCreateFramebuffer failed for layer %d", fb->id, layerIndex);
			return false;
		}
	} else if (texChanged || layerChanged || fb->dirty) {
		fb->dirty = true;
		if (!_priv::buildFboResources(s, *fb)) {
			return false;
		}
	}

	if (s.frameAcquired && (wasBound || s.currentFramebuffer == fbo)) {
		_priv::beginFboRenderPass(s, *fb, clear ? _priv::FboBeginClear::All : _priv::FboBeginClear::Auto);
	}
	return true;
}

bool setupFramebuffer(Id fbo, const TexturePtr (&colorTextures)[core::enumVal(FrameBufferAttachment::Max)],
					  const RenderBufferPtr (&bufferAttachments)[core::enumVal(FrameBufferAttachment::Max)]) {
	_priv::VkState &s = vkstate();
	_priv::VkState::FramebufferHandle *fb = _priv::findFramebufferHandle(s, fbo);
	if (fb == nullptr) {
		return false;
	}
	fb->colorCount = 0u;
	fb->depthAttachment = InvalidId;
	for (int i = 0; i < lengthof(fb->colorAttachments); ++i) {
		fb->colorAttachments[i] = InvalidId;
	}
	const int color0 = core::enumVal(FrameBufferAttachment::Color0);
	for (int i = 0; i < core::enumVal(FrameBufferAttachment::Max); ++i) {
		const TexturePtr &tex = colorTextures[i];
		if (!tex) {
			continue;
		}
		const FrameBufferAttachment att = (FrameBufferAttachment)i;
		if (att == FrameBufferAttachment::Depth || att == FrameBufferAttachment::DepthStencil) {
			fb->depthAttachment = tex->handle();
		} else if (i >= color0) {
			const uint32_t slot = (uint32_t)(i - color0);
			if (slot < lengthof(fb->colorAttachments)) {
				fb->colorAttachments[slot] = tex->handle();
				if (slot + 1u > fb->colorCount) {
					fb->colorCount = slot + 1u;
				}
			}
		}
	}
	for (int i = 0; i < core::enumVal(FrameBufferAttachment::Max); ++i) {
		const RenderBufferPtr &rbo = bufferAttachments[i];
		if (!rbo) {
			continue;
		}
		const FrameBufferAttachment att = (FrameBufferAttachment)i;
		if (att == FrameBufferAttachment::Depth || att == FrameBufferAttachment::DepthStencil) {
			fb->depthAttachment = rbo->handle();
		}
	}
	fb->dirty = true;
	if (!_priv::buildFboResources(s, *fb)) {
		Log::error("setupFramebuffer: build failed for fbo %u", fbo);
		return false;
	}
	return true;
}

void setupTexture(Id texture, const TextureConfig &config) {
	_priv::VkState &s = vkstate();
	_priv::VkState::TextureHandle *h = _priv::findTextureHandle(s, texture);
	if (h == nullptr) {
		return;
	}
	h->type = config.type();
	h->format = config.format();
	h->layers = config.layers();
	h->isDepth = _priv::textureFormatIsDepth(config.format());
}

static VkFilter textureFilterToVk(TextureFilter f) {
	switch (f) {
	case TextureFilter::Nearest:
	case TextureFilter::NearestMipmapNearest:
	case TextureFilter::NearestMipmapLinear:
		return VK_FILTER_NEAREST;
	case TextureFilter::Linear:
	case TextureFilter::LinearMipmapNearest:
	case TextureFilter::LinearMipmapLinear:
	default:
		return VK_FILTER_LINEAR;
	}
}

static VkSamplerAddressMode textureWrapToVk(TextureWrap w) {
	switch (w) {
	case TextureWrap::ClampToBorder:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case TextureWrap::Repeat:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case TextureWrap::MirroredRepeat:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case TextureWrap::ClampToEdge:
	default:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}
}

void uploadTexture(Id texture, int width, int height, const uint8_t *data, int index, const TextureConfig &cfg) {
	statsUploadScopeBegin();
	_priv::VkState &s = vkstate();
	_priv::VkState::TextureHandle *h = _priv::findTextureHandle(s, texture);
	if (h == nullptr || width <= 0 || height <= 0) {
		statsUploadScopeEnd();
		return;
	}
	h->width = width;
	h->height = height;
	h->format = cfg.format();
	h->type = cfg.type();
	h->layers = cfg.layers();
	h->isDepth = _priv::textureFormatIsDepth(cfg.format());

	const VkFormat vkFmt = _priv::textureFormatToVk(cfg.format());
	// Some formats (e.g. R8G8B8_UNORM) are rarely supported for TRANSFER_DST+SAMPLED
	// in optimal tiling. Fall back to R8G8B8A8 by expanding RGB -> RGBA on the CPU.
	bool expandRgbToRgba = false;
	VkFormat actualFmt = vkFmt;
	if (vkFmt == VK_FORMAT_R8G8B8_UNORM) {
		VkFormatProperties fp;
		vkGetPhysicalDeviceFormatProperties(s.physicalDevice, vkFmt, &fp);
		const VkFormatFeatureFlags need =
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR;
		if ((fp.optimalTilingFeatures & need) != need) {
			expandRgbToRgba = true;
			actualFmt = VK_FORMAT_R8G8B8A8_UNORM;
		}
	}

	// Destroy any previous GPU backing (re-upload path).
	if (h->image != VK_NULL_HANDLE || h->view != VK_NULL_HANDLE || h->sampler != VK_NULL_HANDLE) {
		vkQueueWaitIdle(s.graphicsQueue);
		destroyTextureGpu(s, *h);
	}

	const uint32_t bpp = (actualFmt == VK_FORMAT_R8G8B8A8_UNORM && expandRgbToRgba) ? 4u
	                                                                              : _priv::textureFormatBytesPerPixel(cfg.format());
	const VkDeviceSize dataSize = (VkDeviceSize)width * (VkDeviceSize)height * (VkDeviceSize)bpp;
	const TextureType texType = cfg.type();
	const bool isArrayTexture =
		texType == TextureType::Texture2DArray || texType == TextureType::Texture2DMultisampleArray;
	uint32_t arrayLayers = 1u;
	if (isArrayTexture) {
		arrayLayers = (uint32_t)core_max(1, (int)cfg.layers());
		// Allocation-only uploads pass the layer count as index (GL storage3D convention).
		if (data == nullptr && index > 0) {
			arrayLayers = (uint32_t)core_max((int)arrayLayers, index);
		}
	}

	VkImageCreateInfo ici{};
	ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ici.imageType = _priv::ImageTypes[core::enumVal(texType)];
	ici.format = actualFmt;
	ici.extent.width = (uint32_t)width;
	ici.extent.height = (uint32_t)height;
	ici.extent.depth = 1;
	ici.mipLevels = 1;
	ici.arrayLayers = arrayLayers;
	ici.samples = VK_SAMPLE_COUNT_1_BIT;
	ici.tiling = VK_IMAGE_TILING_OPTIMAL;
	ici.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	h->storageBit = false;
	if (!h->isDepth) {
		ici.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		VkFormatProperties fp;
		vkGetPhysicalDeviceFormatProperties(s.physicalDevice, actualFmt, &fp);
		if ((fp.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) != 0) {
			ici.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
			h->storageBit = true;
		}
	} else {
		ici.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if (vkCreateImage(s.device, &ici, nullptr, &h->image) != VK_SUCCESS) {
		Log::error("uploadTexture: vkCreateImage failed (%dx%d fmt=%d)", width, height, (int)actualFmt);
		h->image = VK_NULL_HANDLE;
		statsUploadScopeEnd();
		return;
	}

	VkMemoryRequirements mr;
	vkGetImageMemoryRequirements(s.device, h->image, &mr);
	VkDeviceSize imgOffset = 0;
	bool slab = true;
	if (!_priv::allocGpuMemory(s, mr.size, mr.alignment, mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
							   &h->memory, &imgOffset, nullptr, &slab)) {
		Log::error("uploadTexture: image memory alloc failed (%dx%d)", width, height);
		destroyTextureGpu(s, *h);
		statsUploadScopeEnd();
		return;
	}
	h->slabMemory = slab;
	if (vkBindImageMemory(s.device, h->image, h->memory, imgOffset) != VK_SUCCESS) {
		Log::error("uploadTexture: vkBindImageMemory failed");
		destroyTextureGpu(s, *h);
		statsUploadScopeEnd();
		return;
	}

	// Staging buffer + copy when data is provided. FBO attachments (data ==
	// nullptr) stay UNDEFINED until the first render pass CLEAR.
	VkBuffer staging = VK_NULL_HANDLE;
	uint8_t *expanded = nullptr;
	const uint8_t *uploadData = data;
	VkDeviceSize uploadSize = dataSize;
	if (data != nullptr && expandRgbToRgba) {
		uploadSize = (VkDeviceSize)width * (VkDeviceSize)height * 4u;
		expanded = (uint8_t *)core_malloc((size_t)uploadSize);
		const int px = width * height;
		for (int i = 0; i < px; ++i) {
			expanded[i * 4 + 0] = data[i * 3 + 0];
			expanded[i * 4 + 1] = data[i * 3 + 1];
			expanded[i * 4 + 2] = data[i * 3 + 2];
			expanded[i * 4 + 3] = 255;
		}
		uploadData = expanded;
	}

	const VkImageAspectFlags aspect = _priv::imageAspectForFormat(h->format);
	const VkImageLayout sampleLayout =
		h->isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	if (uploadData != nullptr) {
		VkBufferCreateInfo bci{};
		bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bci.size = uploadSize;
		bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (vkCreateBuffer(s.device, &bci, nullptr, &staging) != VK_SUCCESS) {
			Log::error("uploadTexture: staging buffer create failed");
			if (expanded != nullptr) core_free(expanded);
			destroyTextureGpu(s, *h);
			statsUploadScopeEnd();
			return;
		}
		VkMemoryRequirements bmr;
		vkGetBufferMemoryRequirements(s.device, staging, &bmr);
		VkDeviceMemory stagingMem = VK_NULL_HANDLE;
		void *ptr = nullptr;
		if (!_priv::dedicatedAlloc(s, bmr.size, bmr.memoryTypeBits,
								   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
								   &stagingMem, &ptr)) {
			Log::error("uploadTexture: staging memory alloc failed");
			vkDestroyBuffer(s.device, staging, nullptr);
			if (expanded != nullptr) core_free(expanded);
			destroyTextureGpu(s, *h);
			statsUploadScopeEnd();
			return;
		}
		if (vkBindBufferMemory(s.device, staging, stagingMem, 0) != VK_SUCCESS || ptr == nullptr) {
			Log::error("uploadTexture: staging bind/map failed");
			vkDestroyBuffer(s.device, staging, nullptr);
			vkUnmapMemory(s.device, stagingMem);
			vkFreeMemory(s.device, stagingMem, nullptr);
			if (expanded != nullptr) core_free(expanded);
			destroyTextureGpu(s, *h);
			statsUploadScopeEnd();
			return;
		}
		core_memcpy(ptr, uploadData, (size_t)uploadSize);

		VkCommandBufferAllocateInfo cbai{};
		cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cbai.commandPool = s.commandPool;
		cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cbai.commandBufferCount = 1;
		VkCommandBuffer ccb = VK_NULL_HANDLE;
		if (vkAllocateCommandBuffers(s.device, &cbai, &ccb) != VK_SUCCESS || ccb == VK_NULL_HANDLE) {
			Log::error("uploadTexture: cmd buffer alloc failed");
			vkDestroyBuffer(s.device, staging, nullptr);
			vkUnmapMemory(s.device, stagingMem);
			vkFreeMemory(s.device, stagingMem, nullptr);
			if (expanded != nullptr) core_free(expanded);
			destroyTextureGpu(s, *h);
			statsUploadScopeEnd();
			return;
		}
		VkCommandBufferBeginInfo cbbi{};
		cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(ccb, &cbbi);

		VkImageMemoryBarrier bar{};
		bar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		bar.srcAccessMask = 0;
		bar.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		bar.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		bar.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		bar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bar.image = h->image;
		bar.subresourceRange.aspectMask = aspect;
		bar.subresourceRange.levelCount = 1;
		bar.subresourceRange.layerCount = arrayLayers;
		vkCmdPipelineBarrier(ccb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
							 nullptr, 1, &bar);
		VkBufferImageCopy region{};
		region.imageSubresource.aspectMask = aspect;
		region.imageSubresource.baseArrayLayer = 0u;
		if (isArrayTexture && index >= 0 && index < (int)arrayLayers) {
			region.imageSubresource.baseArrayLayer = (uint32_t)index;
		}
		region.imageSubresource.layerCount = 1;
		region.imageExtent.width = (uint32_t)width;
		region.imageExtent.height = (uint32_t)height;
		region.imageExtent.depth = 1;
		vkCmdCopyBufferToImage(ccb, staging, h->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		bar.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		bar.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		bar.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		bar.newLayout = sampleLayout;
		vkCmdPipelineBarrier(ccb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
							 0, nullptr, 1, &bar);
		vkEndCommandBuffer(ccb);
		VkSubmitInfo si{};
		si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		si.commandBufferCount = 1;
		si.pCommandBuffers = &ccb;
		vkQueueSubmit(s.graphicsQueue, 1, &si, VK_NULL_HANDLE);
		vkQueueWaitIdle(s.graphicsQueue);
		vkFreeCommandBuffers(s.device, s.commandPool, 1, &ccb);
		vkDestroyBuffer(s.device, staging, nullptr);
		vkUnmapMemory(s.device, stagingMem);
		vkFreeMemory(s.device, stagingMem, nullptr);
		h->hasContent = true;
		h->layout = sampleLayout;
	} else if (h->isDepth && arrayLayers > 1u) {
		// Shadow-map arrays are sampled as a whole. Only the layers that a
		// framebuffer 2D view actually renders would otherwise leave UNDEFINED,
		// so clear every layer to far-depth first.
		VkCommandBufferAllocateInfo cbai{};
		cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cbai.commandPool = s.commandPool;
		cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cbai.commandBufferCount = 1;
		VkCommandBuffer ccb = VK_NULL_HANDLE;
		if (vkAllocateCommandBuffers(s.device, &cbai, &ccb) != VK_SUCCESS || ccb == VK_NULL_HANDLE) {
			Log::error("uploadTexture: cmd buffer alloc failed for depth array init");
			destroyTextureGpu(s, *h);
			statsUploadScopeEnd();
			return;
		}
		VkCommandBufferBeginInfo cbbi{};
		cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(ccb, &cbbi);
		VkImageMemoryBarrier bar{};
		bar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		bar.srcAccessMask = 0;
		bar.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		bar.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		bar.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		bar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bar.image = h->image;
		bar.subresourceRange.aspectMask = aspect;
		bar.subresourceRange.levelCount = 1;
		bar.subresourceRange.layerCount = arrayLayers;
		vkCmdPipelineBarrier(ccb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
							 nullptr, 1, &bar);
		VkClearDepthStencilValue clearDepth{};
		clearDepth.depth = 1.0f;
		vkCmdClearDepthStencilImage(ccb, h->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearDepth, 1,
									&bar.subresourceRange);
		bar.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		bar.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		bar.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		bar.newLayout = sampleLayout;
		vkCmdPipelineBarrier(ccb, VK_PIPELINE_STAGE_TRANSFER_BIT,
							 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
								 VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
							 0, 0, nullptr, 0, nullptr, 1, &bar);
		vkEndCommandBuffer(ccb);
		VkSubmitInfo si{};
		si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		si.commandBufferCount = 1;
		si.pCommandBuffers = &ccb;
		vkQueueSubmit(s.graphicsQueue, 1, &si, VK_NULL_HANDLE);
		vkQueueWaitIdle(s.graphicsQueue);
		vkFreeCommandBuffers(s.device, s.commandPool, 1, &ccb);
		h->hasContent = true;
		h->layout = sampleLayout;
	} else {
		h->hasContent = false;
		h->layout = VK_IMAGE_LAYOUT_UNDEFINED;
	}
	if (expanded != nullptr) {
		core_free(expanded);
	}

	// Image view
	VkImageViewCreateInfo ivci{};
	ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ivci.image = h->image;
	ivci.viewType = _priv::ImageViewTypes[core::enumVal(texType)];
	ivci.format = actualFmt;
	ivci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	ivci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	ivci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	ivci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	ivci.subresourceRange.aspectMask = aspect;
	ivci.subresourceRange.levelCount = 1;
	ivci.subresourceRange.layerCount = arrayLayers;
	if (vkCreateImageView(s.device, &ivci, nullptr, &h->view) != VK_SUCCESS) {
		Log::error("uploadTexture: vkCreateImageView failed");
		h->view = VK_NULL_HANDLE;
	}

	// Sampler from TextureConfig.
	VkSamplerCreateInfo sci{};
	sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sci.magFilter = textureFilterToVk(cfg.filterMag());
	sci.minFilter = textureFilterToVk(cfg.filterMin());
	sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sci.addressModeU = textureWrapToVk(cfg.wrapS());
	sci.addressModeV = textureWrapToVk(cfg.wrapT());
	sci.addressModeW = textureWrapToVk(cfg.wrapR());
	sci.anisotropyEnable = VK_FALSE;
	sci.maxAnisotropy = 1.0f;
	if (cfg.compareMode() == TextureCompareMode::RefToTexture && cfg.compareFunc() != CompareFunc::Max) {
		sci.compareEnable = VK_TRUE;
		sci.compareOp = _priv::compareOpFor(cfg.compareFunc());
	} else {
		sci.compareEnable = VK_FALSE;
		sci.compareOp = VK_COMPARE_OP_NEVER;
	}
	sci.minLod = 0.0f;
	sci.maxLod = 0.0f;
	sci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	sci.unnormalizedCoordinates = VK_FALSE;
	if (vkCreateSampler(s.device, &sci, nullptr, &h->sampler) != VK_SUCCESS) {
		Log::error("uploadTexture: vkCreateSampler failed");
		h->sampler = VK_NULL_HANDLE;
	}
	const size_t uploadBytes = (uploadData != nullptr) ? (size_t)uploadSize : 0u;
	if (uploadBytes > 0u) {
		statsBufferUpdate(uploadBytes);
	}
	statsUploadScopeEnd();
}


bool getVulkanTextureHandles(Id handle, void **outView, void **outSampler) {
	_priv::VkState &s = vkstate();
	_priv::VkState::TextureHandle *h = _priv::findTextureHandle(s, handle);
	if (h == nullptr || h->view == VK_NULL_HANDLE || h->sampler == VK_NULL_HANDLE) {
		return false;
	}
	if (outView != nullptr) *outView = h->view;
	if (outSampler != nullptr) *outSampler = h->sampler;
	return true;
}

void *getVulkanImGuiDescriptorSet(Id handle) {
	_priv::VkState &s = vkstate();
	_priv::VkState::TextureHandle *h = _priv::findTextureHandle(s, handle);
	if (h == nullptr || h->imguiSet == VK_NULL_HANDLE) {
		return nullptr;
	}
	return h->imguiSet;
}

void setVulkanImGuiDescriptorSet(Id handle, void *set) {
	_priv::VkState &s = vkstate();
	_priv::VkState::TextureHandle *h = _priv::findTextureHandle(s, handle);
	if (h == nullptr) {
		return;
	}
	if (h->imguiSet != VK_NULL_HANDLE && h->imguiSet != set && s.descriptorPool != VK_NULL_HANDLE) {
		vkFreeDescriptorSets(s.device, s.descriptorPool, 1, &h->imguiSet);
	}
	h->imguiSet = (VkDescriptorSet)set;
}

namespace _priv {

// Compute a stable hash of the VAO's captured vertex input so the pipeline
// cache can invalidate when configuration changes.
static uint64_t hashVertexInput(const VkState::VaoHandle *vao, VkPrimitiveTopology topology) {
	uint64_t h = 14695981039346656037ull; // FNV-1a 64 offset basis
	auto mix = [&](uint64_t v) {
		h ^= v;
		h *= 1099511628211ull;
	};
	mix((uint64_t)topology);
	if (vao == nullptr) {
		return h;
	}
	mix(vao->attributeCount);
	for (uint32_t i = 0; i < vao->attributeCount; ++i) {
		const Attribute &a = vao->attributes[i];
		mix((uint64_t)a.location);
		mix((uint64_t)a.bufferIndex);
		mix((uint64_t)a.size);
		mix((uint64_t)a.stride);
		mix((uint64_t)a.offset);
		mix((uint64_t)core::enumVal(a.type));
		mix((uint64_t)((a.normalized ? 1u : 0u) | (a.typeIsInt ? 2u : 0u) | ((uint32_t)a.divisor << 2)));
	}
	return h;
}

static uint64_t hashPipelineKey(const VkState::VaoHandle *vao, VkPrimitiveTopology topology, bool hasDepth,
							   uint32_t colorCount) {
	uint64_t h = hashVertexInput(vao, topology);
	auto mix = [&](uint64_t v) {
		h ^= v;
		h *= 1099511628211ull;
	};
	const RendererState &rs = rendererState();
	mix(hasDepth ? 1u : 0u);
	mix((uint64_t)colorCount);
	mix(rs.pendingStates[core::enumVal(State::CullFace)] ? 1u : 0u);
	mix((uint64_t)core::enumVal(rs.pendingCullFace));
	mix(rs.pendingStates[core::enumVal(State::Blend)] ? 1u : 0u);
	mix((uint64_t)core::enumVal(rs.pendingBlendSrcRGB));
	mix((uint64_t)core::enumVal(rs.pendingBlendDestRGB));
	mix((uint64_t)core::enumVal(rs.pendingBlendSrcAlpha));
	mix((uint64_t)core::enumVal(rs.pendingBlendDestAlpha));
	mix((uint64_t)core::enumVal(rs.pendingBlendEquation));
	mix(rs.pendingStates[core::enumVal(State::DepthTest)] ? 1u : 0u);
	mix(rs.pendingStates[core::enumVal(State::DepthMask)] ? 1u : 0u);
	mix((uint64_t)core::enumVal(rs.pendingDepthFunc));
	mix(rs.pendingStates[core::enumVal(State::PolygonOffsetFill)] ? 1u : 0u);
	mix(rs.pendingColorMask[0] ? 1u : 0u);
	mix(rs.pendingColorMask[1] ? 1u : 0u);
	mix(rs.pendingColorMask[2] ? 1u : 0u);
	mix(rs.pendingColorMask[3] ? 1u : 0u);
	mix((uint64_t)vkstate().drawBufferMask);
	uint32_t biasX = 0u;
	uint32_t biasY = 0u;
	core_memcpy(&biasX, &rs.pendingPolygonOffset.x, sizeof(biasX));
	core_memcpy(&biasY, &rs.pendingPolygonOffset.y, sizeof(biasY));
	mix((uint64_t)biasX);
	mix((uint64_t)biasY);
	return h;
}

// Allocate a fresh descriptor set for this draw and populate it with the
// currently bound UBO buffers. Returns VK_NULL_HANDLE if the program has no
// UBO bindings (caller must skip the bind step).
//
// The caller must NOT re-use the same VkDescriptorSet across draws: updating
// a set that was previously bound to a recording command buffer invalidates
// the command buffer. Each draw gets its own set and they are released in
// bulk when the owning frame slot cycles back around.

// Bind descriptors for the current program. Uniform buffers use DYNAMIC offsets
// into the per-frame ring so one VkDescriptorSet can be reused across draws that
// only change UBO contents. Returns false if required descriptors cannot be satisfied.
static bool bindDescriptorsForDraw(VkState &s, VkState::ProgramHandle &prog, Id programId, VkCommandBuffer cmd,
								   VkPipelineBindPoint bindPoint) {
	if (prog.descriptorSetLayout == VK_NULL_HANDLE || prog.layout == VK_NULL_HANDLE) {
		return true;
	}

	// Indexed by binding number so dynamic offsets can be emitted in ascending
	// binding order (Vulkan requirement for pDynamicOffsets).
	VkBuffer uboBufferByBinding[16];
	VkDeviceSize uboRangeByBinding[16];
	uint32_t uboOffsetByBinding[16];
	bool uboValidByBinding[16];
	for (uint32_t i = 0; i < 16u; ++i) {
		uboBufferByBinding[i] = VK_NULL_HANDLE;
		uboRangeByBinding[i] = 0u;
		uboOffsetByBinding[i] = 0u;
		uboValidByBinding[i] = false;
	}

	Id textureIds[16];
	for (uint32_t i = 0; i < 16u; ++i) {
		textureIds[i] = (i < (uint32_t)lengthof(s.boundTextures)) ? s.boundTextures[i] : InvalidId;
	}
	const Id imageId = rendererState().imageHandle;

	for (uint32_t bi = 0; bi < prog.registeredBindingCount; ++bi) {
		const ShaderResourceBinding &rb = prog.registeredBindings[bi];
		if (rb.type != ShaderResourceBinding::UniformBuffer || rb.binding >= 16u) {
			continue;
		}
		const uint64_t key =
			(static_cast<uint64_t>(core::enumVal(BufferType::UniformBuffer)) << 32) | (uint64_t)rb.binding;
		Id uboId = InvalidId;
		(void)s.bufferBaseBindings.get(key, uboId);
		if (uboId == InvalidId) {
			continue;
		}
		VkState::BufferHandle *bh = findBufferHandle(s, uboId);
		if (bh == nullptr || bh->data == nullptr || bh->size == 0u) {
			continue;
		}
		VkBuffer buf = VK_NULL_HANDLE;
		VkDeviceSize offset = 0u;
		VkDeviceSize range = 0u;
		if (!snapshotUniformToRing(s, *bh, buf, offset, range)) {
			Log::error("bindDescriptorsForDraw: uniform ring snapshot failed for %s binding %u", prog.name.c_str(),
					   rb.binding);
			return false;
		}
		uboBufferByBinding[rb.binding] = buf;
		uboRangeByBinding[rb.binding] = range;
		uboOffsetByBinding[rb.binding] = (uint32_t)offset;
		uboValidByBinding[rb.binding] = true;

		// Push per-draw overlay fields (registered via setUniformBufferPushOverlay).
		if (bh->pushOverlayFieldCount > 0u && bh->pushConstantSize > 0u && bh->data != nullptr &&
			bindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS && prog.pushConstantSize > 0u) {
			const uint32_t pcSize = core_min(bh->pushConstantSize, prog.pushConstantSize);
			uint8_t stackPc[256];
			uint8_t *pc = stackPc;
			uint8_t *heapPc = nullptr;
			if (pcSize > (uint32_t)sizeof(stackPc)) {
				heapPc = (uint8_t *)core_malloc(pcSize);
				pc = heapPc;
			}
			if (pc != nullptr) {
				core_memset(pc, 0, pcSize);
				for (uint32_t fi = 0; fi < bh->pushOverlayFieldCount; ++fi) {
					const UniformPushOverlayField &f = bh->pushOverlayFields[fi];
					if ((size_t)f.uboOffset + (size_t)f.size > bh->size || f.pushOffset + f.size > pcSize) {
						continue;
					}
					core_memcpy(pc + f.pushOffset, bh->data + f.uboOffset, f.size);
				}
				vkCmdPushConstants(cmd, prog.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, pcSize, pc);
			}
			if (heapPc != nullptr) {
				core_free(heapPc);
			}
		}
	}

	// Compact dynamic offsets in ascending binding order (Vulkan spec).
	VkBuffer uboBuffers[16];
	VkDeviceSize uboRanges[16];
	uint32_t uboDynOffsets[16];
	uint32_t uboCount = 0u;
	for (uint32_t binding = 0; binding < 16u; ++binding) {
		if (!uboValidByBinding[binding]) {
			continue;
		}
		uboBuffers[uboCount] = uboBufferByBinding[binding];
		uboRanges[uboCount] = uboRangeByBinding[binding];
		uboDynOffsets[uboCount] = uboOffsetByBinding[binding];
		++uboCount;
	}

	bool cacheHit = bindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS && s.descCacheSet != VK_NULL_HANDLE &&
					s.descCacheProgramId == programId && s.descCacheImage == imageId && s.descCacheUboCount == uboCount;
	if (cacheHit) {
		for (uint32_t i = 0; i < 16u; ++i) {
			if (s.descCacheTextures[i] != textureIds[i]) {
				cacheHit = false;
				break;
			}
		}
	}
	if (cacheHit) {
		for (uint32_t i = 0; i < uboCount; ++i) {
			if (s.descCacheUboBuffers[i] != uboBuffers[i] || s.descCacheUboRanges[i] != uboRanges[i]) {
				cacheHit = false;
				break;
			}
		}
	}

	VkDescriptorSet set = s.descCacheSet;
	if (!cacheHit) {
		VkDescriptorSetAllocateInfo alloc;
		alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc.pNext = nullptr;
		alloc.descriptorPool = s.descriptorPool;
		alloc.descriptorSetCount = 1;
		alloc.pSetLayouts = &prog.descriptorSetLayout;
		set = VK_NULL_HANDLE;
		VkResult r = vkAllocateDescriptorSets(s.device, &alloc, &set);
		if (r != VK_SUCCESS) {
			Log::error("vkAllocateDescriptorSets failed for %s: %s", prog.name.c_str(), vkResultStr(r));
			return false;
		}

		VkDescriptorBufferInfo bufferInfos[16];
		VkDescriptorImageInfo imageInfos[16];
		VkWriteDescriptorSet writes[16];
		uint32_t writeCount = 0u;
		uint32_t bufferInfoCount = 0u;
		uint32_t imageInfoCount = 0u;

		for (uint32_t bi = 0; bi < prog.registeredBindingCount && writeCount < 16u; ++bi) {
			const ShaderResourceBinding &rb = prog.registeredBindings[bi];
			const uint32_t binding = rb.binding;
			VkWriteDescriptorSet &w = writes[writeCount];
			w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			w.pNext = nullptr;
			w.dstSet = set;
			w.dstBinding = binding;
			w.dstArrayElement = 0;
			w.descriptorCount = 1;
			w.pTexelBufferView = nullptr;
			if (rb.type == ShaderResourceBinding::UniformBuffer) {
				if (binding >= 16u || !uboValidByBinding[binding] || bufferInfoCount >= 16u) {
					Log::error("bindDescriptorsForDraw: missing uniform buffer for %s binding %u", prog.name.c_str(),
							   binding);
					vkFreeDescriptorSets(s.device, s.descriptorPool, 1, &set);
					return false;
				}
				bufferInfos[bufferInfoCount].buffer = uboBufferByBinding[binding];
				bufferInfos[bufferInfoCount].offset = 0u;
				bufferInfos[bufferInfoCount].range = uboRangeByBinding[binding];
				w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				w.pImageInfo = nullptr;
				w.pBufferInfo = &bufferInfos[bufferInfoCount];
				++bufferInfoCount;
			} else if (rb.type == ShaderResourceBinding::StorageImage) {
				Id imgId = imageId;
				if (imgId == InvalidId && binding < (uint32_t)lengthof(s.boundTextures)) {
					imgId = s.boundTextures[binding];
				}
				VkState::TextureHandle *th = findTextureHandle(s, imgId);
				if (th == nullptr || th->view == VK_NULL_HANDLE) {
					continue;
				}
				if (imageInfoCount >= 16u) {
					break;
				}
				imageInfos[imageInfoCount].sampler = VK_NULL_HANDLE;
				imageInfos[imageInfoCount].imageView = th->view;
				imageInfos[imageInfoCount].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				w.pImageInfo = &imageInfos[imageInfoCount];
				w.pBufferInfo = nullptr;
				++imageInfoCount;
			} else {
				VkImageView view = VK_NULL_HANDLE;
				VkSampler sampler = VK_NULL_HANDLE;
				VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				VkState::TextureHandle *th = nullptr;
				if (binding < (uint32_t)lengthof(s.boundTextures)) {
					const Id tid = s.boundTextures[binding];
					if (tid != InvalidId) {
						th = findTextureHandle(s, tid);
						if (th != nullptr && th->view != VK_NULL_HANDLE && th->sampler != VK_NULL_HANDLE &&
							th->layout != VK_IMAGE_LAYOUT_UNDEFINED) {
							view = th->view;
							sampler = th->sampler;
							layout = th->layout;
						}
					}
				}
				if (view == VK_NULL_HANDLE || sampler == VK_NULL_HANDLE) {
					if (th != nullptr && th->type == TextureType::Texture2DArray) {
						view = s.placeholderDepthArrayView;
						sampler = s.placeholderDepthArraySampler;
						layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
					} else {
						view = s.placeholderImageView;
						sampler = s.placeholderSampler;
					}
				}
				if (view == VK_NULL_HANDLE || sampler == VK_NULL_HANDLE) {
					continue;
				}
				if (imageInfoCount >= 16u) {
					break;
				}
				imageInfos[imageInfoCount].sampler = sampler;
				imageInfos[imageInfoCount].imageView = view;
				imageInfos[imageInfoCount].imageLayout = layout;
				w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				w.pImageInfo = &imageInfos[imageInfoCount];
				w.pBufferInfo = nullptr;
				++imageInfoCount;
			}
			++writeCount;
		}
		if (writeCount == 0u) {
			vkFreeDescriptorSets(s.device, s.descriptorPool, 1, &set);
			return false;
		}
		vkUpdateDescriptorSets(s.device, writeCount, writes, 0, nullptr);

		const uint32_t frame = s.currentFrame;
		if (s.pendingFreeSetCount[frame] >= s.pendingFreeSetCapacity[frame]) {
			const uint32_t newCap = s.pendingFreeSetCapacity[frame] == 0u ? 64u : s.pendingFreeSetCapacity[frame] * 2u;
			VkDescriptorSet *newArr = (VkDescriptorSet *)core_malloc(sizeof(VkDescriptorSet) * newCap);
			if (s.pendingFreeSets[frame] != nullptr) {
				core_memcpy(newArr, s.pendingFreeSets[frame], sizeof(VkDescriptorSet) * s.pendingFreeSetCount[frame]);
				core_free(s.pendingFreeSets[frame]);
			}
			s.pendingFreeSets[frame] = newArr;
			s.pendingFreeSetCapacity[frame] = newCap;
		}
		s.pendingFreeSets[frame][s.pendingFreeSetCount[frame]++] = set;

		if (bindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS) {
			s.descCacheSet = set;
			s.descCacheProgramId = programId;
			s.descCacheImage = imageId;
			s.descCacheUboCount = uboCount;
			for (uint32_t i = 0; i < 16u; ++i) {
				s.descCacheTextures[i] = textureIds[i];
				s.descCacheUboBuffers[i] = (i < uboCount) ? uboBuffers[i] : VK_NULL_HANDLE;
				s.descCacheUboRanges[i] = (i < uboCount) ? uboRanges[i] : 0u;
			}
		}
	}

	bool offsetsMatch = cacheHit && s.descBoundSet == set && s.descBoundDynOffsetCount == uboCount;
	if (offsetsMatch) {
		for (uint32_t i = 0; i < uboCount; ++i) {
			if (s.descBoundDynOffsets[i] != uboDynOffsets[i]) {
				offsetsMatch = false;
				break;
			}
		}
	}
	if (offsetsMatch) {
		statsStateChangeSkipped();
		return true;
	}

	vkCmdBindDescriptorSets(cmd, bindPoint, prog.layout, 0, 1, &set, uboCount, uboCount > 0u ? uboDynOffsets : nullptr);
	statsDescriptorBind();
	if (bindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS) {
		s.descBoundSet = set;
		s.descBoundDynOffsetCount = uboCount;
		for (uint32_t i = 0; i < 16u; ++i) {
			s.descBoundDynOffsets[i] = (i < uboCount) ? uboDynOffsets[i] : 0u;
		}
	}
	if (cacheHit) {
		statsStateChangeSkipped();
	}
	return true;
}

// Used by runShader (compute). Binds descriptors immediately; return value unused.
static VkDescriptorSet acquireDescriptorSet(VkState &s, VkState::ProgramHandle &prog) {
	VkCommandBuffer cmd = s.frameCommandBuffers[s.currentFrame];
	if (!bindDescriptorsForDraw(s, prog, prog.id, cmd, VK_PIPELINE_BIND_POINT_COMPUTE)) {
		return VK_NULL_HANDLE;
	}
	return (VkDescriptorSet)(uintptr_t)1; // non-null success sentinel for callers
}

static void releasePendingDescriptorSets(VkState &s, uint32_t frame) {
	if (s.pendingFreeSetCount[frame] == 0u) {
		return;
	}
	vkFreeDescriptorSets(s.device, s.descriptorPool, s.pendingFreeSetCount[frame], s.pendingFreeSets[frame]);
	s.pendingFreeSetCount[frame] = 0u;
}

static void applyPendingViewportScissor(VkState &s, VkCommandBuffer cmd) {
	RendererState &rs = rendererState();
	uint32_t targetW = s.swapchainExtent.width;
	uint32_t targetH = s.swapchainExtent.height;
	if (s.fboRenderPassActive && s.boundFboHandle != InvalidId) {
		VkState::FramebufferHandle *fb = findFramebufferHandle(s, s.boundFboHandle);
		if (fb != nullptr && fb->width > 0u && fb->height > 0u) {
			targetW = fb->width;
			targetH = fb->height;
		}
	}
	int vpX = rs.pendingViewportX;
	int vpY = rs.pendingViewportY;
	int vpW = rs.pendingViewportW;
	int vpH = rs.pendingViewportH;
	if (vpW <= 0 || vpH <= 0) {
		vpX = 0;
		vpY = 0;
		vpW = (int)targetW;
		vpH = (int)targetH;
	}
	if (s.descCacheViewport[0] != vpX || s.descCacheViewport[1] != vpY || s.descCacheViewport[2] != vpW ||
		s.descCacheViewport[3] != vpH) {
		VkViewport vp;
		vp.x = (float)vpX;
		vp.y = (float)(vpY + vpH);
		vp.width = (float)vpW;
		vp.height = -(float)vpH;
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;
		vkCmdSetViewport(cmd, 0, 1, &vp);
		s.descCacheViewport[0] = vpX;
		s.descCacheViewport[1] = vpY;
		s.descCacheViewport[2] = vpW;
		s.descCacheViewport[3] = vpH;
	} else {
		statsStateChangeSkipped();
	}

	// Pending scissor uses the same lower-left origin as GL glScissor().
	int scX = rs.pendingScissorX;
	int scY = rs.pendingScissorY;
	int scW = rs.pendingScissorW;
	int scH = rs.pendingScissorH;
	if (scW <= 0 || scH <= 0) {
		scX = vpX;
		scY = vpY;
		scW = vpW;
		scH = vpH;
	}
	// Intersect with the active viewport so a stale full-window scissor does not
	// produce negative Vulkan offsets when the viewport was narrowed (e.g. FBO).
	const int scRight = scX + scW;
	const int scTop = scY + scH;
	const int vpRight = vpX + vpW;
	const int vpTop = vpY + vpH;
	scX = core_max(scX, vpX);
	scY = core_max(scY, vpY);
	const int scX1 = core_min(scRight, vpRight);
	const int scY1 = core_min(scTop, vpTop);
	scW = scX1 - scX;
	scH = scY1 - scY;
	if (scW <= 0 || scH <= 0) {
		scX = vpX;
		scY = vpY;
		scW = vpW;
		scH = vpH;
	}
	// Vulkan scissor offset is top-left; pending coords are lower-left.
	const int scTopEdge = scY + scH;
	int32_t scissorY = (int32_t)targetH - scTopEdge;
	if (!rs.clipOriginLowerLeft) {
		scissorY = scY;
	}
	VkRect2D sc;
	const float scale = rs.scaleFactor;
	sc.offset.x = (int32_t)((float)scX * scale + 0.5f);
	scissorY = (int32_t)((float)scissorY * scale + 0.5f);
	sc.extent.width = (uint32_t)((float)scW * scale + 0.5f);
	sc.extent.height = (uint32_t)((float)scH * scale + 0.5f);
	if (sc.extent.width == 0u) {
		sc.extent.width = 1u;
	}
	if (sc.extent.height == 0u) {
		sc.extent.height = 1u;
	}
	const int32_t maxY = (int32_t)((float)targetH * scale + 0.5f);
	if (scissorY < 0) {
		if (scissorY + (int32_t)sc.extent.height <= 0) {
			scissorY = 0;
		} else {
			sc.extent.height = (uint32_t)((int32_t)sc.extent.height + scissorY);
			scissorY = 0;
		}
	}
	if (scissorY + (int32_t)sc.extent.height > maxY) {
		const int32_t overflow = scissorY + (int32_t)sc.extent.height - maxY;
		if (scissorY >= maxY) {
			scissorY = core_max(0, maxY - 1);
			sc.extent.height = 1u;
		} else {
			sc.extent.height = (uint32_t)((int32_t)sc.extent.height - overflow);
		}
	}
	sc.offset.y = scissorY;
	if (sc.extent.width == 0u) {
		sc.extent.width = 1u;
	}
	if (sc.extent.height == 0u) {
		sc.extent.height = 1u;
	}
	if (s.descCacheScissor[0] != sc.offset.x || s.descCacheScissor[1] != sc.offset.y ||
		s.descCacheScissor[2] != (int)sc.extent.width || s.descCacheScissor[3] != (int)sc.extent.height) {
		vkCmdSetScissor(cmd, 0, 1, &sc);
		s.descCacheScissor[0] = sc.offset.x;
		s.descCacheScissor[1] = sc.offset.y;
		s.descCacheScissor[2] = (int)sc.extent.width;
		s.descCacheScissor[3] = (int)sc.extent.height;
	} else {
		statsStateChangeSkipped();
	}
}


// Prepare pipeline + descriptor set + bind vertex/index buffers + set
// viewport/scissor. Returns the command buffer to record draw calls into,
// or VK_NULL_HANDLE if the frame is not ready / setup failed.
static VkCommandBuffer prepareDrawState(VkState &s, VkPrimitiveTopology topology) {
	if (!s.frameAcquired) {
		return VK_NULL_HANDLE;
	}
	Id programId = rendererState().pendingProgramHandle;
	if (programId == InvalidId) {
		programId = rendererState().programHandle;
	}
	VkState::ProgramHandle *prog = findProgramHandle(s, programId);
	if (prog == nullptr) {
		return VK_NULL_HANDLE;
	}
	VkState::VaoHandle *vao = findVaoHandle(s, rendererState().vertexArrayHandle);

	VkPipeline pipe = findOrCreatePipeline(s, *prog, vao, topology);
	if (pipe == VK_NULL_HANDLE) {
		return VK_NULL_HANDLE;
	}

	VkCommandBuffer cmd = s.frameCommandBuffers[s.currentFrame];

	applyPendingViewportScissor(s, cmd);

	if (s.boundGraphicsPipeline != pipe) {
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
		s.boundGraphicsPipeline = pipe;
		statsPipelineBind();
	} else {
		statsStateChangeSkipped();
	}

	if (!bindDescriptorsForDraw(s, *prog, programId, cmd, VK_PIPELINE_BIND_POINT_GRAPHICS)) {
		return VK_NULL_HANDLE;
	}

	const Id vaoId = rendererState().vertexArrayHandle;
	if (vao != nullptr && vao->attributeCount > 0u && s.boundVaoId != vaoId) {
		VkBuffer vertexBuffers[16];
		VkDeviceSize offsets[16];
		int seenBufferIndex[16];
		for (int i = 0; i < 16; ++i) {
			seenBufferIndex[i] = -1;
		}
		uint32_t bindingCount = 0u;
		for (uint32_t i = 0; i < vao->attributeCount && bindingCount < 16u; ++i) {
			const Attribute &a = vao->attributes[i];
			bool seen = false;
			for (uint32_t b = 0; b < bindingCount; ++b) {
				if (seenBufferIndex[b] == a.bufferIndex) {
					seen = true;
					break;
				}
			}
			if (seen) {
				continue;
			}
			seenBufferIndex[bindingCount] = a.bufferIndex;
			Id arrId = InvalidId;
			if (a.bufferIndex >= 0 && a.bufferIndex < 16) {
				arrId = vao->arrayBuffers[a.bufferIndex];
			}
			VkState::BufferHandle *bh = findBufferHandle(s, arrId);
			if (bh == nullptr || bh->buffer == VK_NULL_HANDLE) {
				return VK_NULL_HANDLE;
			}
			vertexBuffers[bindingCount] = bh->buffer;
			offsets[bindingCount] = 0;
			++bindingCount;
		}
		if (bindingCount > 0u) {
			vkCmdBindVertexBuffers(cmd, 0, bindingCount, vertexBuffers, offsets);
		}
		s.boundVaoId = vaoId;
	} else if (vao != nullptr && s.boundVaoId == vaoId) {
		statsStateChangeSkipped();
	}

	return cmd;
}

} // namespace _priv

void drawElements(Primitive mode, size_t numIndices, DataType type, void *offset) {
	_priv::VkState &s = vkstate();
	VkCommandBuffer cmd = _priv::prepareDrawState(s, _priv::primitiveTopologyFor(mode));
	if (cmd == VK_NULL_HANDLE) {
		return;
	}
	_priv::VkState::VaoHandle *vao = _priv::findVaoHandle(s, rendererState().vertexArrayHandle);
	Id indexId = vao != nullptr ? vao->indexBuffer : rendererState().bufferHandle[core::enumVal(BufferType::IndexBuffer)];
	_priv::VkState::BufferHandle *ibh = _priv::findBufferHandle(s, indexId);
	if (ibh == nullptr || ibh->buffer == VK_NULL_HANDLE) {
		return;
	}
	vkCmdBindIndexBuffer(cmd, ibh->buffer, 0, _priv::indexTypeFor(type));
	const uint32_t indexOffset = (uint32_t)((uintptr_t)offset / (type == DataType::UnsignedShort ? 2u : 4u));
	vkCmdDrawIndexed(cmd, (uint32_t)numIndices, 1, indexOffset, 0, 0);
	statsDrawCall();
}

void multiDrawElementsIndirect(Primitive mode, DataType type, const void *indirect, int drawCount, int stride) {
	if (drawCount <= 0) {
		return;
	}
	if (!hasFeature(Feature::MultiDrawIndirect)) {
		Log::error("multiDrawElementsIndirect requested without Feature::MultiDrawIndirect");
		return;
	}
	_priv::VkState &s = vkstate();
	VkCommandBuffer cmd = _priv::prepareDrawState(s, _priv::primitiveTopologyFor(mode));
	if (cmd == VK_NULL_HANDLE) {
		return;
	}
	_priv::VkState::VaoHandle *vao = _priv::findVaoHandle(s, rendererState().vertexArrayHandle);
	Id indexId = vao != nullptr ? vao->indexBuffer : rendererState().bufferHandle[core::enumVal(BufferType::IndexBuffer)];
	_priv::VkState::BufferHandle *ibh = _priv::findBufferHandle(s, indexId);
	if (ibh == nullptr || ibh->buffer == VK_NULL_HANDLE) {
		return;
	}
	vkCmdBindIndexBuffer(cmd, ibh->buffer, 0, _priv::indexTypeFor(type));

	const Id indirectId = rendererState().bufferHandle[core::enumVal(BufferType::IndirectBuffer)];
	_priv::VkState::BufferHandle *ind = _priv::findBufferHandle(s, indirectId);
	if (ind == nullptr || ind->buffer == VK_NULL_HANDLE) {
		Log::error("multiDrawElementsIndirect: no IndirectBuffer bound");
		return;
	}
	const VkDeviceSize offset = (VkDeviceSize)(uintptr_t)indirect;
	const uint32_t strideBytes =
		stride > 0 ? (uint32_t)stride : (uint32_t)sizeof(DrawElementsIndirectCommand);
	// One submission for the whole indirect batch (matches GL stats accounting).
	vkCmdDrawIndexedIndirect(cmd, ind->buffer, offset, (uint32_t)drawCount, strideBytes);
	statsDrawCall();
}

void drawArrays(Primitive mode, size_t count) {
	_priv::VkState &s = vkstate();
	VkCommandBuffer cmd = _priv::prepareDrawState(s, _priv::primitiveTopologyFor(mode));
	if (cmd == VK_NULL_HANDLE) {
		return;
	}
	vkCmdDraw(cmd, (uint32_t)count, 1, 0, 0);
	statsDrawCall();
}

void enableDebug(DebugSeverity severity) {
	_priv::VkState &s = vkstate();
	s.debugSeverity = severity;
	if (severity == DebugSeverity::None) {
		Log::info("Vulkan debug messages disabled");
		return;
	}
	if (s.debugReportCallback == VK_NULL_HANDLE && !s.validationEnabled) {
		Log::warn("No Vulkan debug feature support was detected");
		return;
	}
	Log::info("enable vulkan debug messages");
}

bool compileShaderSpirv(Id id, ShaderType shaderType, const uint8_t *spirv, size_t spirvSize,
						const core::String &name) {
	_priv::VkState &s = vkstate();
	_priv::VkState::ShaderHandle *h = _priv::findShaderHandle(s, id);
	if (h == nullptr) {
		Log::error("Invalid shader handle %u for %s", id, name.c_str());
		return false;
	}
	if (h->type != shaderType) {
		Log::warn("Shader handle %u type mismatch for %s", id, name.c_str());
	}
	if (spirv == nullptr || spirvSize == 0) {
		return false;
	}
	if (spirvSize < 4 || spirvSize % 4 != 0) {
		Log::error("Invalid SPIR-V binary size %u for %s (must be non-zero and multiple of 4)", (unsigned)spirvSize,
				   name.c_str());
		return false;
	}
	const uint32_t magic = *(const uint32_t *)spirv;
	if (magic != 0x07230203) {
		Log::error("SPIR-V magic mismatch for %s (got 0x%08x)", name.c_str(), magic);
		return false;
	}

	VkShaderModuleCreateInfo createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.codeSize = spirvSize;
	createInfo.pCode = (const uint32_t *)spirv;

	VkResult result = vkCreateShaderModule(s.device, &createInfo, nullptr, &h->module);
	if (result != VK_SUCCESS) {
		Log::error("vkCreateShaderModule failed for %s: %s", name.c_str(), _priv::vkResultStr(result));
		return false;
	}
	Log::debug("Created VkShaderModule for %s from %u SPIR-V bytes", name.c_str(), (unsigned)spirvSize);
	return true;
}

bool compileShader(Id id, ShaderType shaderType, const core::String &source, const core::String &name) {
	_priv::VkState &s = vkstate();
	_priv::VkState::ShaderHandle *h = _priv::findShaderHandle(s, id);
	if (h == nullptr) {
		Log::error("Invalid shader handle %u for %s", id, name.c_str());
		return false;
	}
	if (h->type != shaderType) {
		Log::warn("Shader handle %u type mismatch for %s", id, name.c_str());
	}
	if (source.empty()) {
		Log::error("Empty shader source for %s", name.c_str());
		return false;
	}
	h->source = source;

	// GLSL fallback path. Without a runtime GLSL -> SPIR-V compiler linked in, we
	// cannot produce a VkShaderModule here. If compileShaderSpirv() was called first
	// the module is already in place. Otherwise the caller (linkShader) will fail to
	// create a VkPipeline and fall back to the non-draw stub path. This is only hit
	// when shadertool's glslangValidator step did not produce a .spv for this stage.
	if (h->module == VK_NULL_HANDLE) {
		Log::error(
			"Shader %s has no VkShaderModule (no SPIR-V was embedded); Vulkan cannot compile GLSL at runtime",
			name.c_str());
		return false;
	}
	return true;
}

bool loadShaderSPIRV(Id id, ShaderType shaderType, const uint8_t *spirv, size_t spirvSize, const core::String &name) {
	return compileShaderSpirv(id, shaderType, spirv, spirvSize, name);
}

void registerShaderBindings(Id program, const ShaderResourceBinding *bindings, int count) {
	_priv::VkState &s = vkstate();
	_priv::VkState::ProgramHandle *prog = _priv::findProgramHandle(s, program);
	if (prog == nullptr) {
		return;
	}
	prog->registeredBindingCount = 0u;
	for (int i = 0; i < count && (uint32_t)i < _priv::VkState::ProgramHandle::MAX_BINDINGS; ++i) {
		prog->registeredBindings[i] = bindings[i];
		++prog->registeredBindingCount;
	}
	// Bindings define the descriptor set layout; rebuild pipelines/layout next draw.
	_priv::invalidateProgramPipelines(s, *prog, true);
}

void setProgramPushConstantSize(Id program, uint32_t size) {
	_priv::VkState &s = vkstate();
	_priv::VkState::ProgramHandle *prog = _priv::findProgramHandle(s, program);
	if (prog == nullptr) {
		return;
	}
	// Push-constant range sizes must be a multiple of 4.
	const uint32_t aligned = (size + 3u) & ~3u;
	if (prog->pushConstantSize == aligned && prog->layout != VK_NULL_HANDLE) {
		return;
	}
	prog->pushConstantSize = aligned;
	// Pipelines reference the old pipeline layout; destroy them before the layout.
	_priv::invalidateProgramPipelines(s, *prog, false);
}

void setUniformBufferPushOverlay(Id buffer, const UniformPushOverlayField *fields, int count) {
	_priv::VkState &s = vkstate();
	_priv::VkState::BufferHandle *bh = _priv::findBufferHandle(s, buffer);
	if (bh == nullptr || fields == nullptr || count <= 0) {
		return;
	}
	const uint32_t n =
		(uint32_t)core_min(count, (int)_priv::VkState::BufferHandle::MAX_PUSH_OVERLAY_FIELDS);
	bh->pushOverlayFieldCount = 0u;
	bh->pushConstantSize = 0u;
	for (uint32_t i = 0; i < n; ++i) {
		bh->pushOverlayFields[i] = fields[i];
		const uint32_t end = fields[i].pushOffset + fields[i].size;
		if (end > bh->pushConstantSize) {
			bh->pushConstantSize = end;
		}
		++bh->pushOverlayFieldCount;
	}
	bh->pushConstantSize = (bh->pushConstantSize + 3u) & ~3u;
	// Insertion-sort by uboOffset so stable compares can walk linearly.
	for (uint32_t i = 1; i < bh->pushOverlayFieldCount; ++i) {
		UniformPushOverlayField key = bh->pushOverlayFields[i];
		int32_t j = (int32_t)i - 1;
		while (j >= 0 && bh->pushOverlayFields[j].uboOffset > key.uboOffset) {
			bh->pushOverlayFields[j + 1] = bh->pushOverlayFields[j];
			--j;
		}
		bh->pushOverlayFields[j + 1] = key;
	}
	bh->stableRingFrame = ~0u;
	bh->stableRingBuffer = VK_NULL_HANDLE;
}

bool linkShader(Id program, Id vert, Id frag, Id geom, const core::String &name) {
	_priv::VkState &s = vkstate();
	_priv::VkState::ProgramHandle *prog = _priv::findProgramHandle(s, program);
	if (prog == nullptr) {
		Log::error("Invalid program handle %u for %s", program, name.c_str());
		return false;
	}

	_priv::VkState::ShaderHandle *vertH = _priv::findShaderHandle(s, vert);
	_priv::VkState::ShaderHandle *fragH = _priv::findShaderHandle(s, frag);
	if (vertH == nullptr || fragH == nullptr) {
		Log::error("Missing vertex/fragment shader handles for %s", name.c_str());
		return false;
	}
	if (geom != InvalidId) {
		_priv::VkState::ShaderHandle *geomH = _priv::findShaderHandle(s, geom);
		if (geomH == nullptr || geomH->module == VK_NULL_HANDLE) {
			Log::error("Invalid geometry shader handle for %s", name.c_str());
			return false;
		}
		prog->geomShader = geom;
	}

	// Create render pass if not already created
	if (s.renderPass == VK_NULL_HANDLE) {
		if (!_priv::createRenderPass(s.device, s.swapchainFormat, s.glowFormat, s.renderPass)) {
			Log::error("Failed to create render pass for %s", name.c_str());
			return false;
		}
	}

	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout layout = VK_NULL_HANDLE;
	// Pipeline creation is deferred until first draw: we need the full
	// reflection data (registered AFTER linkShader by shadertool's setup())
	// plus the vertex input layout captured via configureAttribute() to build
	// a valid VkPipeline. Creating a hardcoded pipeline here produces validation
	// errors (wrong vertex input, missing descriptor set layouts, mismatched
	// color attachment count) and is never actually bound at draw time anyway.
	(void)vertH;
	(void)fragH;
	(void)pipeline;

	prog->vertShader = vert;
	prog->fragShader = frag;
	prog->layout = layout;
	prog->name = name;
	Log::debug("linkShader %s (pipeline deferred to first draw)", name.c_str());
	return true;
}

bool linkComputeShader(Id program, Id comp, const core::String &name) {
	_priv::VkState &s = vkstate();
	_priv::VkState::ProgramHandle *prog = _priv::findProgramHandle(s, program);
	if (prog == nullptr) {
		Log::error("Invalid program handle %u for %s", program, name.c_str());
		return false;
	}
	_priv::VkState::ShaderHandle *compH = _priv::findShaderHandle(s, comp);
	if (compH == nullptr || compH->module == VK_NULL_HANDLE) {
		Log::error("Missing compute shader handle for %s", name.c_str());
		return false;
	}
	prog->computeShader = comp;
	prog->name = name;
	Log::debug("linkComputeShader %s (pipeline deferred to first dispatch)", name.c_str());
	return true;
}

bool bindImage(Id handle, AccessMode mode, ImageFormat format) {
	if (rendererState().imageHandle == handle && rendererState().imageFormat == format &&
		rendererState().imageAccessMode == mode) {
		return false;
	}
	rendererState().imageHandle = handle;
	rendererState().imageFormat = format;
	rendererState().imageAccessMode = mode;
	return true;
}

static void transitionStorageImage(VkCommandBuffer cmd, _priv::VkState::TextureHandle *th, VkImageLayout newLayout,
								   VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage,
								   VkPipelineStageFlags dstStage) {
	if (th == nullptr || th->image == VK_NULL_HANDLE || th->layout == newLayout) {
		return;
	}
	cmdBlitImageBarrier(cmd, th->image, th->layout, newLayout, srcAccess, dstAccess, srcStage, dstStage,
						VK_IMAGE_ASPECT_COLOR_BIT);
	th->layout = newLayout;
}

void waitShader(MemoryBarrierType wait) {
	if (wait == MemoryBarrierType::None) {
		return;
	}
	_priv::VkState &s = vkstate();
	if (!s.frameAcquired || s.mainRenderPassActive || s.fboRenderPassActive) {
		return;
	}
	if (wait != MemoryBarrierType::ShaderImageAccess && wait != MemoryBarrierType::All) {
		return;
	}
	_priv::VkState::TextureHandle *th = _priv::findTextureHandle(s, rendererState().imageHandle);
	if (th == nullptr || th->image == VK_NULL_HANDLE) {
		return;
	}
	VkCommandBuffer cmd = s.frameCommandBuffers[s.currentFrame];
	transitionStorageImage(cmd, th, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_WRITE_BIT,
						   VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	th->hasContent = true;
}

bool runShader(Id program, const glm::uvec3 &workGroups, MemoryBarrierType wait) {
	if (workGroups.x == 0u || workGroups.y == 0u || workGroups.z == 0u) {
		return false;
	}
	if (!checkLimit((int)workGroups.x, Limit::MaxComputeWorkGroupCountX)) {
		return false;
	}
	if (!checkLimit((int)workGroups.y, Limit::MaxComputeWorkGroupCountY)) {
		return false;
	}
	if (!checkLimit((int)workGroups.z, Limit::MaxComputeWorkGroupCountZ)) {
		return false;
	}

	video::useProgram(program);
	video::syncPendingState();

	_priv::VkState &s = vkstate();
	if (!s.frameAcquired) {
		Log::error("runShader: no recording command buffer");
		return false;
	}
	_priv::VkState::ProgramHandle *prog = _priv::findProgramHandle(s, program);
	if (prog == nullptr) {
		Log::error("runShader: invalid program %u", program);
		return false;
	}
	if (!_priv::ensureComputePipeline(s, *prog)) {
		return false;
	}

	_priv::VkState::TextureHandle *th = _priv::findTextureHandle(s, rendererState().imageHandle);
	if (th == nullptr || th->image == VK_NULL_HANDLE || th->view == VK_NULL_HANDLE) {
		Log::error("runShader: no storage image bound");
		return false;
	}
	if (!th->storageBit) {
		Log::error("runShader: texture %u has no STORAGE usage (format=%d)", th->id, (int)th->format);
		return false;
	}

	const Id savedFbo = s.boundFboHandle;
	const bool resumeFbo = s.fboRenderPassActive;
	if (resumeFbo) {
		_priv::endFboRenderPassIfActive(s);
	} else if (s.mainRenderPassActive) {
		// vkCmdDispatch is invalid inside a render pass. Advance overlay then end,
		// matching blitFramebuffer, so the resume path can start subpass 0 again.
		_priv::endMainRenderPassIfActive(s);
	}

	VkCommandBuffer cmd = s.frameCommandBuffers[s.currentFrame];
	VkAccessFlags srcAccess = 0;
	VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	if (th->layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		srcAccess = VK_ACCESS_SHADER_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (th->layout == VK_IMAGE_LAYOUT_GENERAL) {
		srcAccess = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	} else if (th->layout != VK_IMAGE_LAYOUT_UNDEFINED) {
		srcAccess = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}
	transitionStorageImage(cmd, th, VK_IMAGE_LAYOUT_GENERAL, srcAccess, VK_ACCESS_SHADER_WRITE_BIT, srcStage,
						   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, prog->computePipeline);
	if (prog->descriptorSetLayout != VK_NULL_HANDLE) {
		if (_priv::acquireDescriptorSet(s, *prog) == VK_NULL_HANDLE) {
			Log::error("runShader: failed to bind compute descriptors for %s", prog->name.c_str());
			if (resumeFbo && savedFbo != InvalidId) {
				_priv::VkState::FramebufferHandle *fb = _priv::findFramebufferHandle(s, savedFbo);
				if (fb != nullptr) {
					_priv::beginFboRenderPass(s, *fb, _priv::FboBeginClear::Auto);
				}
			} else {
				_priv::resumeMainRenderPass(s);
			}
			return false;
		}
	}
	vkCmdDispatch(cmd, workGroups.x, workGroups.y, workGroups.z);
	waitShader(wait);

	if (resumeFbo && savedFbo != InvalidId) {
		_priv::VkState::FramebufferHandle *fb = _priv::findFramebufferHandle(s, savedFbo);
		if (fb != nullptr) {
			_priv::beginFboRenderPass(s, *fb, _priv::FboBeginClear::Auto);
		}
	} else {
		_priv::resumeMainRenderPass(s);
	}
	return true;
}

void setObjectName(Id handle, ObjectNameType type, const core::String &name) {
}

void flush() {
}

void finish() {
}

void traceVideoBegin(const char *name) {
	core::traceBegin(name);
}

void traceVideoEnd() {
	core::traceEnd();
}

void *mapBufferRange(Id handle, BufferType type, intptr_t offset, size_t length, AccessMode mode, MapBufferFlag flags) {
	_priv::VkState &s = vkstate();
	_priv::VkState::BufferHandle *h = _priv::findBufferHandle(s, handle);
	if (h == nullptr) {
		return nullptr;
	}
	h->type = type;
	// Ensure the backing VkBuffer is large enough to cover [offset, offset+length).
	const size_t needed = (size_t)offset + length;
	if (needed > h->size) {
		h->size = needed;
	}
	if (!_priv::ensureBufferAlloc(s, *h, h->size)) {
		return nullptr;
	}
	if (h->mapped == nullptr) {
		return nullptr;
	}
	return (void *)((uint8_t *)h->mapped + offset);
}

void *mapBuffer(Id handle, BufferType type, AccessMode mode) {
	_priv::VkState &s = vkstate();
	_priv::VkState::BufferHandle *h = _priv::findBufferHandle(s, handle);
	if (h == nullptr) {
		return nullptr;
	}
	h->type = type;
	if (h->size == 0u) {
		return nullptr;
	}
	if (!_priv::ensureBufferAlloc(s, *h, h->size)) {
		return nullptr;
	}
	return h->mapped;
}

bool unmapBuffer(Id handle, BufferType type) {
	// Buffers are persistently mapped; nothing to do. HOST_COHERENT memory means
	// writes through @c mapped are visible to the device without explicit flushes.
	(void)handle;
	(void)type;
	return true;
}

void *getVulkanInstance() {
	return vkstate().instance;
}

void *getVulkanInstanceProcAddr(const char *name) {
	if (name == nullptr || vkstate().instance == VK_NULL_HANDLE) {
		return nullptr;
	}
	return (void *)vkGetInstanceProcAddr(vkstate().instance, name);
}

void *getVulkanPhysicalDevice() {
	return vkstate().physicalDevice;
}

void *getVulkanDevice() {
	return vkstate().device;
}

uint32_t getVulkanDeviceQueueFamily() {
	return vkstate().graphicsQueueFamily;
}

void *getVulkanDeviceQueue() {
	return vkstate().graphicsQueue;
}

void *getVulkanRenderPass() {
	return vkstate().renderPass;
}

uint32_t getVulkanMinImageCount() {
	return vkstate().swapchainMinImageCount;
}

uint32_t getVulkanImageCount() {
	return vkstate().swapchainImageCount;
}

void *getVulkanDescriptorPool() {
	return vkstate().descriptorPool;
}

void *getVulkanCommandBuffer() {
	_priv::VkState &s = vkstate();
	if (!s.frameAcquired) {
		return nullptr;
	}
	return s.frameCommandBuffers[s.currentFrame];
}

void nextSubpass() {
	_priv::VkState &s = vkstate();
	if (!s.frameAcquired || s.advancedToOverlaySubpass) {
		return;
	}
	VkCommandBuffer cmd = s.frameCommandBuffers[s.currentFrame];
	vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
	s.advancedToOverlaySubpass = true;
}

} // namespace video
