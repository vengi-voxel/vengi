/**
 * @file
 */

#pragma once

#include "core/NonCopyable.h"
#include "core/String.h"
#include "core/TimeProvider.h"
#include "image/Image.h"
#include "scenegraph/SceneGraph.h"
#include "video/Camera.h"
#include "video/OpenFileMode.h"
#include "voxel/MeshState.h"
#include "voxelrender/RenderContext.h"
#include "voxelrender/SceneGraphRenderer.h"
#include "math/Vec2i.h"
#include "math/Vec2f.h"

namespace io {
struct FilesystemEntry;
struct FormatDescription;
} // namespace io

namespace voxelui {

/**
 * @brief Reusable ImGui scene preview using @c voxelrender::SceneGraphRenderer.
 *
 * Orbit (left-drag) and zoom (wheel) around the scene center. Usable from panels
 * and as a file-dialog preview callback.
 */
class ScenePreview : public core::NonCopyable {
private:
	core::TimeProviderPtr _timeProvider;
	scenegraph::SceneGraph _sceneGraph;
	voxel::MeshStatePtr _meshState;
	voxelrender::SceneGraphRenderer _renderer;
	voxelrender::RenderContext _renderContext;
	video::Camera _camera;
	core::String _loadedPath;
	math::Vec2i _fbSize{0};
	bool _initialized = false;
	bool _cameraDirty = true;

	bool ensureInit();
	void resetCamera();
	void resize(const math::Vec2i &pixelSize);
	void renderFrame(double deltaSeconds);
	void handleOrbitInput(const math::Vec2f &size);
	void drawImage(const math::Vec2f &size) const;
	math::Vec2i pixelSizeFor(const math::Vec2f &widgetSize) const;
	const video::FrameBuffer &displayFrameBuffer() const;

public:
	explicit ScenePreview(const core::TimeProviderPtr &timeProvider);
	~ScenePreview();

	bool init();
	void shutdown();

	void clear();
	bool empty() const;
	const core::String &loadedPath() const;

	/**
	 * @brief Load a voxel model from @p path (filesystem or app search path).
	 * @return @c false if the path is empty, unsupported, or load failed
	 */
	bool load(const core::String &path);
	/**
	 * @brief Replace the preview scene (takes ownership of @p sceneGraph)
	 */
	bool setSceneGraph(scenegraph::SceneGraph &&sceneGraph);

	/**
	 * @brief Render the current scene into the preview framebuffer (no ImGui).
	 */
	void render(double deltaSeconds, const math::Vec2i &pixelSize);
	/**
	 * @brief Color attachment shown in the preview (resolve FBO when MSAA is on).
	 */
	image::ImagePtr image() const;
	bool multisamplingEnabled() const;

	/**
	 * @brief Draw an interactive preview of the given size (logical ImGui units).
	 */
	void updateAndRender(double deltaSeconds, const math::Vec2f &size, bool interactive = true);

	/**
	 * @brief File-dialog hook: load selected voxel file (if any) and draw a preview child.
	 */
	void renderForFileDialog(const io::FilesystemEntry &entry, video::OpenFileMode mode,
							 const io::FormatDescription *desc);
};

} // namespace voxelui
