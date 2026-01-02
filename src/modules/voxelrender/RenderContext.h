/**
 * @file
 */

#pragma once

#include "core/NonCopyable.h"
#include "render/BloomRenderer.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "video/FrameBuffer.h"
#include <glm/vec2.hpp>

namespace scenegraph {
class SceneGraph;
}

namespace voxelrender {

enum class RenderMode : uint8_t {
	Edit, Scene, Max
};

struct RenderContext : public core::NonCopyable {
	video::FrameBuffer frameBuffer;        // Main framebuffer (multisampled when MSAA is enabled)
	video::FrameBuffer resolveFrameBuffer; // Resolve target for multisampled framebuffer
	render::BloomRenderer bloomRenderer;
	const scenegraph::SceneGraph *sceneGraph = nullptr;
	scenegraph::FrameIndex frame = 0;
	bool hideInactive = false;
	bool grayInactive = false;
	bool onlyModels = false;
	// render the built-in normals
	bool renderNormals = false;
	bool applyTransformsInEditMode = true;
	RenderMode renderMode = RenderMode::Edit;
	// multisampling configuration
	bool enableMultisampling = false;
	int multisampleSamples = 4;

	bool isEditMode() const;
	bool isSceneMode() const;
	bool showCameras() const;
	bool applyTransforms() const;

	bool init(const glm::ivec2 &size);
	void shutdown();
	bool resize(const glm::ivec2 &size);
	bool updateMultisampling();
};

} // namespace voxelrender
