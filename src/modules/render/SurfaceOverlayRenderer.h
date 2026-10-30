/**
 * @file
 */

#pragma once

#include "SurfaceoverlayShader.h"
#include "SurfaceoverlayData.h"
#include "video/Buffer.h"
#include "video/FrameBuffer.h"
#include <glm/vec4.hpp>

namespace render {

/**
 * @brief Composite prospective surfaces over a scene without coplanar depth writes.
 * begin() captures the current target's depth and binds an isolated color/depth
 * target. Render overlay geometry there, then call end() to restore the scene.
 * Own one instance per viewport; targets are allocated lazily and reused.
 * The source framebuffer must have D32F depth, matching the copied depth target.
 */
class SurfaceOverlayRenderer : public core::NonCopyable {
private:
	shader::SurfaceoverlayShader _shader;
	shader::SurfaceoverlayData _data;
	alignas(16) shader::SurfaceoverlayData::FragData _fragData{};
	video::Buffer _quad;
	video::FrameBuffer _surfaces;
	video::FrameBuffer _sceneDepth;
	int _position = -1;
	bool resize(const glm::ivec2 &size);

public:
	bool begin(const glm::ivec2 &size);
	/** A nonzero tint alpha enables a hatched intent overlay in the supplied color. */
	void end(const glm::vec4 &tint = glm::vec4(0.0f));
	void shutdown();
};

} // namespace render
