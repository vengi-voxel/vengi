/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "imgui.h"
#include "video/WindowedApp.h"
#include "video/gl/GLTypes.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "voxel/Region.h"
#include "voxelrender/RawVolumeRenderer.h"

namespace voxelformat {
class SceneGraphNode;
}

struct ImGuiDockNode;

namespace voxedit {

class Viewport {
public:
	enum class SceneCameraMode : uint8_t {
		Free, Top, Left, Front
	};
private:
	const core::String _id;
	bool _hovered = false;
	bool _guizmoActivated = false;

	int _mouseX = 0;
	int _mouseY = 0;

	struct Bounds {
		glm::vec3 mins{0};
		glm::vec3 maxs{0};
	};
	Bounds _boundsNode;
	Bounds _bounds;

	voxelrender::RenderContext _renderContext;
	core::VarPtr _showAxisVar;
	core::VarPtr _guizmoRotation;
	core::VarPtr _guizmoAllowAxisFlip;
	core::VarPtr _guizmoSnap;
	core::VarPtr _viewDistance;

	SceneCameraMode _camMode = SceneCameraMode::Free;
	core::VarPtr _rotationSpeed;
	video::Camera _camera;

	void renderToFrameBuffer();
	bool setupFrameBuffer(const glm::ivec2& frameBufferSize);
	void renderSceneGuizmo(video::Camera &camera);
	void renderCameraManipulator(video::Camera &camera, float headerSize);
	void renderGizmo(video::Camera &camera, float headerSize, const ImVec2 &size);
	void updateViewportTrace(float headerSize);
	bool isFixedCamera() const;
	void renderViewportImage(const glm::ivec2 &contentSize);
	void dragAndDrop(float headerSize);
	void bottomBar();
	void renderViewport();
	void renderMenuBar(command::CommandExecutionListener *listener);
	void menuBarCameraMode();
public:
	Viewport(const core::String& id);
	~Viewport();

	void resetCamera(const glm::ivec3 &pos, const voxel::Region &region);

	void resize(const glm::ivec2& frameBufferSize);

	void move(bool pan, bool rotate, int x, int y);

	video::Camera& camera();

	bool isHovered() const;
	/**
	 * Update the ui
	 */
	void update(command::CommandExecutionListener *listener);
	bool init(Viewport::SceneCameraMode mode);
	void shutdown();

	const core::String& id() const;

	void resetCamera();
	bool saveImage(const char* filename);
};

inline video::Camera& Viewport::camera() {
	return _camera;
}

inline const core::String& Viewport::id() const {
	return _id;
}

inline bool Viewport::isHovered() const {
	return _hovered;
}

}
