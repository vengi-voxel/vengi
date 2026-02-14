/**
 * @file
 */

#pragma once

#include "image/CaptureTool.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIEx.h"
#include "ui/Panel.h"
#include "video/Camera.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "voxelrender/RenderUtil.h"

namespace io {
class FileStream;
}

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

class Viewport : public ui::Panel {
private:
	using Super = ui::Panel;

	const int _id;
	const core::String _uiId;
	const bool _detailedTitle;
	// was the view manipulator used to change the camera?
	bool _cameraManipulated = false;
	// is this viewport instance hovered at the moment?
	bool _hovered = false;
	// is this viewport instance visible at all?
	bool _visible = false;
	bool _viewportUIElementHovered = false;
	/**
	 * while we are still modifying the transform or shifting the volume we don't want to
	 * flood the memento states - thus we lock the memento handler and track this here.
	 * @sa lock()
	 * @sa unlock()
	 * @sa reset()
	 */
	bool _transformMementoLocked = false;
	glm::mat4x4 _transformLocalMatrix{1.0f};

	math::Axis _sliceAxis = math::Axis::Y;

	voxelrender::SceneCameraMode _camMode = voxelrender::SceneCameraMode::Free;
	image::CaptureTool _captureTool;
	SceneManagerPtr _sceneMgr;

	/**
	 * @sa lock()
	 * @sa unlock()
	 */
	void reset();
	/**
	 * @param[in] keyFrameIdx Only given when we are modifying a transform in scene mode - in edit mode this
	 * should be @c InvalidKeyFrame
	 * @sa lock()
	 * @sa reset()
	 */
	void unlock(const scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx = InvalidKeyFrame);
	/**
	 * @param[in] keyFrameIdx Only given when we are modifying a transform in scene mode - in edit mode this
	 * should be @c InvalidKeyFrame
	 * @sa unlock()
	 * @sa reset()
	 */
	void lock(const scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx = InvalidKeyFrame);

	double _nowSeconds = 0.0;
	double _resizeRequestSeconds = 1.0;
	glm::ivec2 _resizeRequestSize{0, 0};
	glm::ivec2 _pos{0};
	glm::ivec2 _size{0};

	struct Bounds {
		glm::vec3 mins{0};
		glm::vec3 maxs{0};
	};
	Bounds _boundsNode;
	Bounds _bounds;

	voxelrender::RenderContext _renderContext;
	video::Camera _camera;

	core::VarPtr _showAxisVar;
	core::VarPtr _gizmoOperations;
	core::VarPtr _gizmoAllowAxisFlip;
	core::VarPtr _gizmoSnap;
	core::VarPtr _modelGizmo;
	core::VarPtr _viewDistance;
	core::VarPtr _viewMode;
	core::VarPtr _cursorDetails;
	core::VarPtr _pivotMode;
	core::VarPtr _hideInactive;
	core::VarPtr _gridSize;
	core::VarPtr _autoKeyFrame;
	core::VarPtr _localSpace;
	core::VarPtr _renderNormals;
	core::VarPtr _animationPlaying;
	core::VarPtr _clipping;

	bool wantGizmo() const;
	/**
	 * @brief Create a reference of the given mode node by holding shift and clicking on the gizmo
	 */
	bool createReference(const scenegraph::SceneGraphNode &node) const;
	glm::mat4 gizmoMatrix(const scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex &keyFrameIdx) const;
	void updateBounds(const scenegraph::SceneGraphNode &node);
	const float *gizmoBounds(const scenegraph::SceneGraphNode &node);
	bool gizmoManipulate(const video::Camera &camera, const float *boundsPtr, glm::mat4 &matrix, glm::mat4 &deltaMatrix,
						 uint32_t operation) const;
	uint32_t gizmoMode() const;
	uint32_t gizmoOperation(const scenegraph::SceneGraphNode &node) const;
	void renderToFrameBuffer();
	bool setupFrameBuffer(const glm::ivec2 &frameBufferSize);
	void updateGizmoValues(const scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx,
						   const glm::mat4 &matrix);
	/**
	 * See the return value documentation of @c renderGizmo()
	 * @sa renderGizmo()
	 */
	bool runGizmo(const video::Camera &camera);
	void manipulatePivot(scenegraph::SceneGraphNode &node, const glm::mat4 &deltaMatrix);
	void manipulateNodeTransform(const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
								 scenegraph::KeyFrameIndex &keyFrameIdx, const glm::mat4 &worldMatrix);
	void manipulateNodeVolumeRegion(scenegraph::SceneGraphNode &node, const glm::mat4 &worldMatrix);
	void renderCameraManipulator(video::Camera &camera, float headerSize);
	/**
	 * @return @c true if the the gizmo was used in edit mode.
	 * @note This does not return @c true if the gizmo was used in scene mode. This is due to the fact that the edit
	 * mode volume trace has to be reset when we activate the gizmo. Otherwise you would span an aabb for the modifier
	 * to get executed in
	 */
	bool renderGizmo(video::Camera &camera, float headerSize, const ImVec2 &size);
	void updateViewportInput(float headerSize);
	bool isFixedCamera() const;
	void renderViewportImage(const glm::ivec2 &contentSize);
	void dragAndDrop(float headerSize);
	void renderCursor();
	void renderCursorDetails() const;
	bool renderSlicer(const glm::ivec2 &contentSize);
	void renderViewport();
	void toggleVideoRecording();
	void menuBarPolygonModeOptions();
	void menuBarCaptureOptions();
	void menuBarScreenshotOptions(command::CommandExecutionListener *listener);
	void menuBarView(command::CommandExecutionListener *listener);
	void menuBarRenderModeToggle();
	void menuBarMementoOptions(command::CommandExecutionListener *listener);
	void renderMenuBar(command::CommandExecutionListener *listener);
	void resize(const glm::ivec2 &contentSize);
	void resizeCamera(const glm::ivec2 &contentSize);
	void delayResize(const glm::ivec2 &contentSize);
	glm::vec2 dpiScale() const;
	image::ImagePtr renderToImage(const char *imageName);
	void setRenderMode(voxelrender::RenderMode renderMode);

public:
	Viewport(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr, int id, voxelrender::RenderMode renderMode,
			 bool detailedTitle = true);
	~Viewport();

	static core::String viewportId(int id, bool printable = false);

	void toggleScene();
	bool isSceneMode() const;
	bool isGameMode() const;

	video::Camera &camera();
	const video::Camera &camera() const;

	bool isHovered() const;
	bool isVisible() const;
	/**
	 * Update the ui
	 */
	void update(double nowSeconds, command::CommandExecutionListener *listener);
	bool init();
	void shutdown();

	int id() const;
	const glm::ivec2 &pos() const;
	const glm::ivec2 &size() const;

	void resetCamera();
	bool saveImage(const char *filename);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

inline const video::Camera &Viewport::camera() const {
	return _camera;
}

inline video::Camera &Viewport::camera() {
	return _camera;
}

inline int Viewport::id() const {
	return _id;
}

inline bool Viewport::isHovered() const {
	return _hovered && !_cameraManipulated && !_viewportUIElementHovered;
}

inline bool Viewport::isVisible() const {
	return _visible;
}

inline const glm::ivec2 &Viewport::pos() const {
	return _pos;
}

inline const glm::ivec2 &Viewport::size() const {
	return _size;
}

} // namespace voxedit
