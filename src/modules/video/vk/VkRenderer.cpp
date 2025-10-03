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
#include "video/vk/VkState.h"
#include <SDL_vulkan.h>

namespace video {

static inline _priv::VkState &vkstate() {
	static _priv::VkState s;
	return s;
}

void setup() {
}

bool init(int windowWidth, int windowHeight, float scaleFactor) {
	if (flextVkInit() == -1) {
		Log::error("Could not initialize opengl: %s", SDL_GetError());
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
		const char **extensions = (const char **)core_malloc(sizeof(const char *) * extensionCount);
		SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions);
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
		core_free(names);
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

	const core::VarPtr &multisampleBuffers = core::Var::getSafe(cfg::ClientMultiSampleBuffers);
	const core::VarPtr &multisampleSamples = core::Var::getSafe(cfg::ClientMultiSampleSamples);
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

void endFrame(SDL_Window *window) {
}

bool checkError(bool triggerAssert) {
	// Vulkan errors are checked immediately when calling functions
	// This is mostly for API compatibility with OpenGL
	return false;
}

void readBuffer(GBufferTextureType textureType) {
}

bool blendFuncSeparate(BlendMode srcRGB, BlendMode destRGB, BlendMode srcAlpha, BlendMode destAlpha) {
	return false;
}

bool pointSize(float size) {
	return false;
}

float lineWidth(float width) {
	return 1.0f;
}

float currentLineWidth() {
	return vkstate().lineWidth;
}

bool clearColor(const glm::vec4 &clearColor) {
	return false;
}

const glm::vec4 &currentClearColor() {
	return vkstate().clearColor;
}

void clear(ClearFlag flag) {
}

bool viewport(int x, int y, int w, int h) {
	return false;
}

void getScissor(int &x, int &y, int &w, int &h) {
	x = vkstate().scissorX;
	y = vkstate().scissorY;
	w = vkstate().scissorW;
	h = vkstate().scissorH;
}

void getViewport(int &x, int &y, int &w, int &h) {
	x = vkstate().viewportX;
	y = vkstate().viewportY;
	w = vkstate().viewportW;
	h = vkstate().viewportH;
}

bool scissor(int x, int y, int w, int h) {
	return false;
}

bool enable(State state) {
	return false;
}

bool currentState(State state) {
	return vkstate().states[core::enumVal(state)];
}

bool disable(State state) {
	return false;
}

void colorMask(bool red, bool green, bool blue, bool alpha) {
}

bool cullFace(Face face) {
	return false;
}

bool depthFunc(CompareFunc func) {
	return false;
}

CompareFunc getDepthFunc() {
	return vkstate().depthFunc;
}

void getBlendState(bool &enabled, BlendMode &src, BlendMode &dest, BlendEquation &func) {
	enabled = vkstate().states[core::enumVal(State::Blend)];
	src = vkstate().blendSrcRGB;
	dest = vkstate().blendDestRGB;
	func = vkstate().blendEquation;
}

bool blendFunc(BlendMode src, BlendMode dest) {
	return false;
}

bool blendEquation(BlendEquation func) {
	return false;
}

PolygonMode polygonMode(Face face, PolygonMode mode) {
	return PolygonMode::Max;
}

bool polygonOffset(const glm::vec2 &offset) {
	return false;
}

Id currentTexture(TextureUnit unit) {
	return vkstate().textureHandle[core::enumVal(unit)];
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

Id boundVertexArray() {
	return vkstate().vertexArrayHandle;
}

Id boundBuffer(BufferType type) {
	return vkstate().bufferHandle[core::enumVal(type)];
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

const core::DynamicSet<Id> &textures() {
	static core::DynamicSet<Id> todo;
	return todo;
}

void deleteTextures(uint8_t amount, Id *ids) {
}

Id currentFramebuffer() {
	return vkstate().framebufferHandle;
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

Face currentCullFace() {
	return vkstate().cullFace;
}

} // namespace video
