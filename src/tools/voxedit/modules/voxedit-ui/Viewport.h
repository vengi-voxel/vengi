/**
 * @file
 */

#pragma once

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

namespace voxedit {

class Viewport {
public:
	enum class SceneCameraMode : uint8_t {
		Free, Top, Left, Front
	};

	enum class RenderMode {
		Editor,
		Max
	};
private:
	const core::String _id;
	bool _hovered = false;
	bool _hoveredGuizmoLastFrame = false;
	bool _guizmoActivated = false;
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

	float _angle = 0.0f;
	SceneCameraMode _camMode = SceneCameraMode::Free;
	core::VarPtr _rotationSpeed;
	video::Camera _camera;
	RenderMode _renderMode = RenderMode::Editor;

	void renderToFrameBuffer();
	bool setupFrameBuffer(const glm::ivec2& frameBufferSize);
	void renderSceneGuizmo(video::Camera &camera);
	void renderCameraManipulator(video::Camera &camera);
	void renderGizmo(video::Camera &camera, const float headerSize, const ImVec2 &size);
	void updateViewportTrace(float headerSize);
	bool isFixedCamera() const;
public:
	Viewport(const core::String& id);
	~Viewport();

	bool _mouseDown = false;
	int _mouseX = 0;
	int _mouseY = 0;

	void setMode(SceneCameraMode mode);
	void resetCamera(const glm::ivec3 &pos, const voxel::Region &region);

	RenderMode renderMode() const;
	void setRenderMode(RenderMode mode);

	void resize(const glm::ivec2& frameBufferSize);

	void update(double deltaFrameSeconds);

	void move(bool pan, bool rotate, int x, int y);

	video::Camera& camera();

	float angle() const;
	void setAngle(float angle);
	bool isHovered() const;
	void update();
	bool init(RenderMode renderMode = RenderMode::Editor);
	void shutdown();

	const core::String& id() const;

	void resetCamera();
	bool saveImage(const char* filename);
};


inline Viewport::RenderMode Viewport::renderMode() const {
	return _renderMode;
}

inline void Viewport::setRenderMode(RenderMode renderMode) {
	_renderMode = renderMode;
}

inline video::Camera& Viewport::camera() {
	return _camera;
}

inline float Viewport::angle() const {
	return _angle;
}

inline void Viewport::setAngle(float angle) {
	_angle = angle;
}

inline const core::String& Viewport::id() const {
	return _id;
}

inline bool Viewport::isHovered() const {
	return _hovered;
}

}
