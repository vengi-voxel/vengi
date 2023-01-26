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
#include "voxelformat/SceneGraphNode.h"
#include "voxelrender/RawVolumeRenderer.h"

namespace voxelformat {
class SceneGraphNode;
}

struct ImGuiDockNode;

namespace voxedit {

class Viewport {
private:
	enum class SceneCameraMode : uint8_t {
		Free, Top, Left, Front, Max
	};
	static constexpr const char *SceneCameraModeStr[] = {"Free", "Top", "Left", "Front"};
	static_assert(lengthof(SceneCameraModeStr) == (int)SceneCameraMode::Max, "Array size doesn't match enum values");
	const int _id;
	const core::String _uiId;
	const bool _detailedTitle;
	bool _hovered = false;
	bool _guizmoActivated = false;
	bool _transformMementoLocked = false;

	void reset();
	void unlock(voxelformat::SceneGraphNode &node, voxelformat::KeyFrameIndex keyFrameIdx);
	void lock(voxelformat::SceneGraphNode &node, voxelformat::KeyFrameIndex keyFrameIdx);

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
	core::VarPtr _guizmoBounds;
	core::VarPtr _viewDistance;
	core::VarPtr _simplifiedView;

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
	void renderBottomBar();
	void renderViewport();
	void renderMenuBar(command::CommandExecutionListener *listener);
	void menuBarCameraMode();
	void menuBarCameraProjection();
	void resetCamera(const glm::vec3 &pos, float distance, const voxel::Region &region);
	void resize(const glm::ivec2& frameBufferSize);
	void move(bool pan, bool rotate, int x, int y);
public:
	Viewport(int id, bool sceneMode, bool detailedTitle = true);
	~Viewport();

	static core::String viewportId(int id);

	void toggleScene();
	bool isSceneMode() const;

	video::Camera& camera();

	bool isHovered() const;
	/**
	 * Update the ui
	 */
	void update(command::CommandExecutionListener *listener);
	bool init();
	void shutdown();

	const core::String& id() const;

	void resetCamera();
	bool saveImage(const char* filename);
};

inline video::Camera& Viewport::camera() {
	return _camera;
}

inline const core::String& Viewport::id() const {
	return _uiId;
}

inline bool Viewport::isHovered() const {
	return _hovered;
}

}
