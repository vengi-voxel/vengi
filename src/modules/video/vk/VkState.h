/**
 * @file
 */

#pragma once

#include "core/collection/BitSet.h"
#include "video/Types.h"
#include "flextVk.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace video {
namespace _priv {

struct VkState {
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

	core::BitSet<core::enumVal(State::Max)> states;

	// Viewport and scissor
	int viewportX = 0;
	int viewportY = 0;
	int viewportW = 0;
	int viewportH = 0;

	int scissorX = 0;
	int scissorY = 0;
	int scissorW = 0;
	int scissorH = 0;

	// Clear color
	glm::vec4 clearColor{0.0f, 0.0f, 0.0f, 1.0f};

	// Face culling
	Face cullFace = Face::Back;

	// Depth testing
	CompareFunc depthFunc = CompareFunc::Less;

	// Blending
	BlendMode blendSrcRGB = BlendMode::One;
	BlendMode blendDestRGB = BlendMode::Zero;
	BlendMode blendSrcAlpha = BlendMode::One;
	BlendMode blendDestAlpha = BlendMode::Zero;
	BlendEquation blendEquation = BlendEquation::Add;

	// Polygon mode
	PolygonMode polygonMode = PolygonMode::Solid;
	Face polygonModeFace = Face::FrontAndBack;

	// Polygon offset
	glm::vec2 polygonOffset{0.0f, 0.0f};

	// Point size
	float pointSize = 1.0f;

	// Line width
	float lineWidth = 1.0f;

	// Texture handles
	Id textureHandle[core::enumVal(TextureUnit::Max)]{InvalidId};
	TextureUnit textureUnit = TextureUnit::Zero;

	// Program handle
	Id programHandle = InvalidId;

	// Vertex array handle
	Id vertexArrayHandle = InvalidId;

	// Buffer handles
	Id bufferHandle[core::enumVal(BufferType::Max)]{InvalidId};

	// Framebuffer handles
	Id framebufferHandle = InvalidId;
	FrameBufferMode framebufferMode = FrameBufferMode::Default;

	// Renderbuffer handle
	Id renderBufferHandle = InvalidId;

	// Scale factor for high-dpi displays
	float scaleFactor = 1.0f;

	// Whether validation layers are enabled
	bool validationLayersEnabled = false;

	// Whether we need to validate the pipeline state
	bool needValidation = true;

	// Vendor detection
	core::BitSet<core::enumVal(Vendor::Max)> vendor;
};

} // namespace _priv
} // namespace video
