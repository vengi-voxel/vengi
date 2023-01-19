/**
 * @file
 *
 * Some great tips here: https://developer.nvidia.com/opengl-vulkan
 */

#include "VkRenderer.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/Var.h"
#include "flextVk.h"
#include "video/Renderer.h"
#include "video/Types.h"
#include "video/gl/GLTypes.h"
#include <SDL.h>
#include <SDL_vulkan.h>

namespace video {

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
		unsigned int count;
		SDL_Vulkan_GetInstanceExtensions(window, &count, nullptr);
		const char **names = (const char **)core_malloc(sizeof(const char *) * count);
		SDL_Vulkan_GetInstanceExtensions(window, &count, names);

		VkInstanceCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.pApplicationInfo = nullptr;
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
		createInfo.enabledExtensionCount = count;
		createInfo.ppEnabledExtensionNames = names;

		vkCreateInstance(&createInfo, nullptr, &instance);
		core_free(names);
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

	core_free(physicalDevices);

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
	SDL_Vulkan_CreateSurface(window, instance, &surface);

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
	return false;
}

bool setupCubemap(Id &handle, const image::ImagePtr images[6]) {
	return false;
}

void readBuffer(GBufferTextureType textureType) {
}

bool setupGBuffer(Id fbo, const glm::ivec2 &dimension, Id *textures, int texCount, Id depthTexture) {
	return false;
}

float lineWidth(float width) {
	return 1.0f;
}

bool clearColor(const glm::vec4 &clearColor) {
	return false;
}

const glm::vec4 &currentClearColor() {
	static glm::vec4 todo;
	return todo;
}

void clear(ClearFlag flag) {
}

bool viewport(int x, int y, int w, int h) {
	return false;
}

void getScissor(int &x, int &y, int &w, int &h) {
}

void getViewport(int &x, int &y, int &w, int &h) {
}

bool scissor(int x, int y, int w, int h) {
	return false;
}

bool enable(State state) {
	return false;
}

bool currentState(State state) {
	return false;
}

bool disable(State state) {
	return false;
}

bool isClipOriginLowerLeft() {
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
	return CompareFunc::Max;
}

bool setupStencil(const StencilConfig &config) {
	return false;
}

void getBlendState(bool &enabled, BlendMode &src, BlendMode &dest, BlendEquation &func) {
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

bool activateTextureUnit(TextureUnit unit) {
	return false;
}

Id currentTexture(TextureUnit unit) {
	return InvalidId;
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
	return InvalidId;
}

Id boundBuffer(BufferType type) {
	return InvalidId;
}

void unmapBuffer(Id handle, BufferType type) {
}

void *mapBuffer(Id handle, BufferType type, AccessMode mode) {
	return nullptr;
}

void *mapBufferRange(Id handle, intptr_t offset, size_t length, BufferType type, AccessMode mode) {
	return nullptr;
}

bool bindBuffer(BufferType type, Id handle) {
	return false;
}

bool unbindBuffer(BufferType type) {
	return false;
}

bool bindBufferBase(BufferType type, Id handle, uint32_t index) {
	return false;
}

uint8_t *bufferStorage(BufferType type, size_t size) {
	return nullptr;
}

void genBuffers(uint8_t amount, Id *ids) {
}

void deleteBuffers(uint8_t amount, Id *ids) {
}

void genVertexArrays(uint8_t amount, Id *ids) {
}

IdPtr genSync() {
	return InvalidIdPtr;
}

bool waitForClientSync(IdPtr id, uint64_t timeout, bool syncFlushCommands) {
	return false;
}

bool waitForSync(IdPtr id) {
	return false;
}

void deleteSync(IdPtr &id) {
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

const core::Set<Id> &textures() {
	static core::Set<Id> todo;
	return todo;
}

void deleteTextures(uint8_t amount, Id *ids) {
}

Id currentFramebuffer() {
	return InvalidId;
}

void genFramebuffers(uint8_t amount, Id *ids) {
}

void deleteFramebuffers(uint8_t amount, Id *ids) {
}

bool readFramebuffer(int x, int y, int w, int h, TextureFormat format, uint8_t **pixels) {
	return false;
}

void genRenderbuffers(uint8_t amount, Id *ids) {
}

void deleteRenderbuffers(uint8_t amount, Id *ids) {
}

void configureAttribute(const Attribute &a) {
}

Id genOcclusionQuery() {
	return InvalidId;
}

void deleteOcclusionQuery(Id &id) {
}

bool isOcclusionQuery(Id id) {
	return false;
}

bool beginOcclusionQuery(Id id) {
	return false;
}

bool endOcclusionQuery(Id id) {
	return false;
}

bool isOcclusionQueryAvailable(Id id) {
	return false;
}

int getOcclusionQueryResult(Id id, bool wait) {
	return -1;
}

Id bindFramebuffer(Id handle, FrameBufferMode mode) {
	return InvalidId;
}

void blitFramebuffer(Id handle, Id target, ClearFlag flag, int width, int height) {
}

bool setupRenderBuffer(TextureFormat format, int w, int h, int samples) {
	return false;
}

Id bindRenderbuffer(Id handle) {
	return InvalidId;
}

void bufferData(Id handle, BufferType type, BufferMode mode, const void *data, size_t size) {
}

void bufferSubData(Id handle, BufferType type, intptr_t offset, const void *data, size_t size) {
}

size_t bufferSize(BufferType type) {
	return 0;
}

void setupDepthCompareTexture(video::TextureType type, CompareFunc func, TextureCompareMode mode) {
}

const glm::vec4 &framebufferUV() {
	static glm::vec4 todo;
	return todo;
}

bool bindFrameBufferAttachment(Id texture, FrameBufferAttachment attachment, int layerIndex, bool clear) {
	return false;
}

bool setupFramebuffer(const TexturePtr (&colorTextures)[core::enumVal(FrameBufferAttachment::Max)], const RenderBufferPtr (&bufferAttachments)[core::enumVal(FrameBufferAttachment::Max)]) {
	return false;
}

void setupTexture(const TextureConfig &config) {
}

void uploadTexture(video::TextureType type, video::TextureFormat format, int width, int height, const uint8_t *data, int index, int samples) {
}

void drawElements(Primitive mode, size_t numIndices, DataType type, void *offset) {
}

void drawElementsInstanced(Primitive mode, size_t numIndices, DataType type, size_t amount) {
}

void drawElementsBaseVertex(Primitive mode, size_t numIndices, DataType type, size_t indexSize, int baseIndex, int baseVertex) {
}

void drawElementsIndirect(Primitive mode, DataType type, const void *offset) {
}

void drawMultiElementsIndirect(Primitive mode, DataType type, const void *offset, size_t commandSize, size_t stride) {
}

void drawArraysIndirect(Primitive mode, void *offset) {
}

void drawMultiArraysIndirect(Primitive mode, void *offset, size_t commandSize, size_t stride) {
}

void drawArrays(Primitive mode, size_t count) {
}

void drawInstancedArrays(Primitive mode, size_t count, size_t amount) {
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

bool runShader(Id program, const glm::uvec3 &workGroups, bool wait) {
	return false;
}

int fetchUniforms(Id program, ShaderUniforms &uniforms, const core::String &name) {
	return -1;
}

int fetchAttributes(Id program, ShaderAttributes &attributes, const core::String &name) {
	return -1;
}

void flush() {
}

void finish() {
}

bool beginTransformFeedback(Primitive primitive) {
	return false;
}

void pauseTransformFeedback() {
}

void resumeTransformFeedback() {
}

void endTransformFeedback() {
}

bool bindTransforFeebackBuffer(int index, Id bufferId) {
	return false;
}

bool bindTransformFeedback(Id id) {
	return false;
}

void deleteTransformFeedback(Id &id) {
}

Id genTransformFeedback() {
	return InvalidId;
}

bool bindTransformFeedbackVaryings(Id program, TransformFeedbackCaptureMode transformFormat, const core::List<core::String> &transformVaryings) {
	return false;
}

} // namespace video
