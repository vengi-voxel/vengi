/**
 * @file
 */

#pragma once

#include "core/NonCopyable.h"
#include "core/String.h"
#include "core/TimeProvider.h"
#include "scenegraph/SceneGraph.h"
#include "video/Camera.h"
#include "video/OpenFileMode.h"
#include "voxel/MeshState.h"
#include "voxelrender/RenderContext.h"
#include "voxelrender/SceneGraphRenderer.h"
#include <glm/vec2.hpp>

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
	glm::ivec2 _fbSize{0};
	bool _initialized = false;
	bool _cameraDirty = true;

	bool ensureInit();
	void resetCamera();
	void resize(const glm::ivec2 &pixelSize);
	void renderFrame(double deltaSeconds);
	void handleOrbitInput(const glm::vec2 &size);
	void drawImage(const glm::vec2 &size) const;
	glm::ivec2 pixelSizeFor(const glm::vec2 &widgetSize) const;

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
	 * @brief Draw an interactive preview of the given size (logical ImGui units).
	 */
	void updateAndRender(double deltaSeconds, const glm::vec2 &size, bool interactive = true);

	/**
	 * @brief File-dialog hook: load selected voxel file (if any) and draw a preview child.
	 */
	void renderForFileDialog(const io::FilesystemEntry &entry, video::OpenFileMode mode,
							 const io::FormatDescription *desc);
};

} // namespace voxelui
