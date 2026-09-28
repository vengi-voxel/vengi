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
	video::FrameBuffer oitFrameBuffer; // Weighted blended OIT accum + reveal (non-MSAA)
	const scenegraph::SceneGraph *sceneGraph = nullptr;
	scenegraph::FrameIndex frame = 0;
	bool hideInactive = false;
	bool grayInactive = false;
	bool onlyModels = false;
	// Per-target bloom switch (FBOs stay allocated). Callers combine this with cl_bloom.
	bool enableBloom = true;
	bool sceneHasGlow = false;
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
	bool hasOit() const;

	bool init(const glm::ivec2 &size);
	void shutdown();
	bool resize(const glm::ivec2 &size);
	bool updateMultisampling();
	/**
	 * Resolve MSAA Color0, Color1, and depth into resolveFrameBuffer. No-op without MSAA.
	 * Color1 (emit/glow) is blitted separately so Color0 is not copied onto it.
	 */
	void resolveMultisampling();
	/**
	 * Composite bloom onto the displayed color target. Must run after MSAA resolve and
	 * before overlay (grid, cursor, gizmos) rendering so those stay unbloomed.
	 */
	void applyBloom();
	/**
	 * Bind the displayed color target for overlays after resolve/bloom. MSAA switches
	 * from the multisample FBO to the resolve FBO (without clearing). Restricts draws
	 * to Color0 so overlays cannot write the glow attachment.
	 */
	void beginOverlays();
	/**
	 * Restore Color0+Color1 draw buffers and unbind the framebuffer used for this frame.
	 */
	void endFrame();
};

} // namespace voxelrender
