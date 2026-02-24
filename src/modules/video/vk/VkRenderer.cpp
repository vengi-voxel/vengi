/**
 * @file
 *
 * Some great tips here: https://developer.nvidia.com/opengl-vulkan
 */

#include "VkRenderer.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/Trace.h"
#include "core/Var.h"
#include "flextVk.h"
#include "video/Renderer.h"
#include "video/Types.h"
#include <SDL_vulkan.h>

namespace video {
namespace _priv {

struct VkState : public RenderState {
	VkInstance instance = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;
	VkCommandPool commandPool = VK_NULL_HANDLE;
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;

	uint32_t graphicsQueueFamily = UINT32_MAX;
	uint32_t presentQueueFamily = UINT32_MAX;
};

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

bool init(int windowWidth, int windowHeight, float scaleFactor) {
	if (flextVkInit() == -1) {
		Log::error("Could not initialize vulkan: %s", SDL_GetError());
		return false;
	}

	SDL_Window *window = nullptr;

	VkInstance instance;
	{
		unsigned int extensionCount;
#if SDL_VERSION_ATLEAST(3, 2, 0)
		const char *const *extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
#else
		SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
		char **extensions = (char **)core_malloc(sizeof(char *) * extensionCount);
		SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, (const char **)extensions);
#endif

		for (unsigned int i = 0; i < extensionCount; ++i) {
			Log::info("  [%u]: %s", i, extensions[i]);
		}

		VkInstanceCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.pApplicationInfo = nullptr;
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
		createInfo.enabledExtensionCount = extensionCount;
		createInfo.ppEnabledExtensionNames = extensions;

		vkCreateInstance(&createInfo, nullptr, &instance);
#if SDL_VERSION_ATLEAST(3, 2, 0)
#else
		core_free((void *)extensions);
#endif
	}

	uint32_t physicalDeviceCount;
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
	VkPhysicalDevice *physicalDevices = (VkPhysicalDevice *)core_malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices);

	VkDeviceQueueCreateInfo queueCreateInfo;
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.pNext = nullptr;
	queueCreateInfo.flags = 0;
	queueCreateInfo.queueFamilyIndex = 0;
	queueCreateInfo.queueCount = 1;
	const float queuePriority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkDeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = nullptr;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = nullptr;
	deviceCreateInfo.enabledExtensionCount = 0;
	deviceCreateInfo.ppEnabledExtensionNames = nullptr;
	deviceCreateInfo.pEnabledFeatures = nullptr;

	VkDevice device;
	vkCreateDevice(physicalDevices[0], &deviceCreateInfo, nullptr, &device);

	core_free((void *)physicalDevices);

	VkQueue queue;
	vkGetDeviceQueue(device, 0, 0, &queue);

	VkCommandPoolCreateInfo commandPoolCreateInfo;
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext = nullptr;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = 0;

	VkCommandPool commandPool;
	vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);

	VkCommandBufferAllocateInfo allocateInfo;
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.pNext = nullptr;
	allocateInfo.commandPool = commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer);

	VkSurfaceKHR surface;
#if SDL_VERSION_ATLEAST(3, 2, 0)
	SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface);
#else
	SDL_Vulkan_CreateSurface(window, instance, &surface);
#endif

	if (useFeature(Feature::DirectStateAccess)) {
		Log::debug("Use direct state access");
	} else {
		Log::debug("No direct state access");
	}

	const core::VarPtr &multisampleBuffers = core::Var::getVar(cfg::ClientMultiSampleBuffers);
	const core::VarPtr &multisampleSamples = core::Var::getVar(cfg::ClientMultiSampleSamples);
	bool multisampling = multisampleSamples->intVal() > 0 && multisampleBuffers->intVal() > 0;

	if (multisampling) {
		video::enable(video::State::MultiSample);
	}

	return true;
}

void resize(int windowWidth, int windowHeight, float scaleFactor) {
}

float getScaleFactor() {
	return 1.0f;
}

glm::ivec2 getWindowSize() {
	return glm::ivec2(-1);
}

void destroyContext(RendererContext &context) {
}

RendererContext createContext(SDL_Window *window) {
	return nullptr;
}

void activateContext(SDL_Window *window, RendererContext &context) {
}

void startFrame(SDL_Window *window, RendererContext &context) {
}

void syncPendingState() {
}

void endFrame(SDL_Window *window) {
}

bool checkError(bool triggerAssert) {
	// Vulkan errors are checked immediately when calling functions
	// This is mostly for API compatibility with OpenGL
	return false;
}

void readBuffer(GBufferTextureType textureType) {
}

float lineWidth(float width) {
	return 1.0f;
}

void clear(ClearFlag flag) {
}

bool bindTexture(TextureUnit unit, TextureType type, Id handle) {
	return false;
}

bool readTexture(TextureUnit unit, TextureType type, TextureFormat format, Id handle, int w, int h, uint8_t **pixels) {
	return false;
}

bool useProgram(Id handle) {
	return false;
}

Id getProgram() {
	return InvalidId;
}

bool bindVertexArray(Id handle) {
	return false;
}

bool bindBuffer(BufferType type, Id handle) {
	return false;
}

bool unbindBuffer(BufferType type) {
	return bindBuffer(type, InvalidId);
}

bool bindBufferBase(BufferType type, Id handle, uint32_t index) {
	return false;
}

void genBuffers(uint8_t amount, Id *ids) {
}

void deleteBuffers(uint8_t amount, Id *ids) {
}

void genVertexArrays(uint8_t amount, Id *ids) {
}

void deleteShader(Id &id) {
}

Id genShader(ShaderType type) {
	return InvalidId;
}

void deleteProgram(Id &id) {
}

Id genProgram() {
	return InvalidId;
}

void deleteVertexArrays(uint8_t amount, Id *ids) {
}

void deleteVertexArray(Id &id) {
}

void genTextures(const TextureConfig &cfg, uint8_t amount, Id *ids) {
}

void deleteTextures(uint8_t amount, Id *ids) {
}

void genFramebuffers(uint8_t amount, Id *ids) {
}

void deleteFramebuffers(uint8_t amount, Id *ids) {
}

void genRenderbuffers(uint8_t amount, Id *ids) {
}

void deleteRenderbuffers(uint8_t amount, Id *ids) {
}

void configureAttribute(const Attribute &a) {
}

Id bindFramebuffer(Id handle, FrameBufferMode mode) {
	return InvalidId;
}

void blitFramebuffer(Id handle, Id target, ClearFlag flag, int width, int height) {
}

bool setupRenderBuffer(Id rbo, TextureFormat format, int w, int h, int samples) {
	return false;
}

Id bindRenderbuffer(Id handle) {
	return InvalidId;
}

void bufferData(Id handle, BufferType type, BufferMode mode, const void *data, size_t size) {
}

void bufferSubData(Id handle, BufferType type, intptr_t offset, const void *data, size_t size) {
}

const glm::vec4 &framebufferUV() {
	static glm::vec4 todo;
	return todo;
}

bool bindFrameBufferAttachment(Id fbo, Id texture, FrameBufferAttachment attachment, int layerIndex, bool clear) {
	return false;
}

bool setupFramebuffer(Id fbo, const TexturePtr (&colorTextures)[core::enumVal(FrameBufferAttachment::Max)],
					  const RenderBufferPtr (&bufferAttachments)[core::enumVal(FrameBufferAttachment::Max)]) {
	return false;
}

void setupTexture(Id texture, const TextureConfig &config) {
}

void uploadTexture(Id texture, int width, int height, const uint8_t *data, int index, const TextureConfig &cfg) {
}

void drawElements(Primitive mode, size_t numIndices, DataType type, void *offset) {
}

void drawArrays(Primitive mode, size_t count) {
}

void enableDebug(DebugSeverity severity) {
}

bool compileShader(Id id, ShaderType shaderType, const core::String &source, const core::String &name) {
	return false;
}

bool linkShader(Id program, Id vert, Id frag, Id geom, const core::String &name) {
	return false;
}

bool linkComputeShader(Id program, Id comp, const core::String &name) {
	return false;
}

bool bindImage(Id handle, AccessMode mode, ImageFormat format) {
	return false;
}

void waitShader(MemoryBarrierType wait) {
}

bool runShader(Id program, const glm::uvec3 &workGroups, MemoryBarrierType wait) {
	return false;
}

int fetchUniforms(Id program, ShaderUniforms &uniforms, const core::String &name) {
	return -1;
}

int fetchAttributes(Id program, ShaderAttributes &attributes, const core::String &name) {
	return -1;
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
	return nullptr;
}

void *mapBuffer(Id handle, BufferType type, AccessMode mode) {
	return nullptr;
}

void unmapBuffer(Id handle, BufferType type) {
}

void setUniformBufferBinding(Id program, uint32_t blockIndex, uint32_t blockBinding) {
}

void setUniformi(int location, int value) {
}

int32_t getUniformBufferOffset(Id program, const char *name) {
	return -1;
}

} // namespace video
