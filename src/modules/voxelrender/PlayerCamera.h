/**
 * @file
 */

#pragma once

#include "video/Camera.h"
#include "voxelworld/WorldMgr.h"
#include "WorldRenderer.h"
#include "core/command/ActionButton.h"
#include "core/BindingContext.h"

namespace voxelrender {

/**
 * @brief The player camera clips against the world while moving.
 */
class PlayerCamera {
private:
	video::Camera _camera;
	voxelworld::WorldMgrPtr _worldMgr;
	WorldRenderer& _worldRenderer;

	core::VarPtr _maxTargetDistance;
	core::VarPtr _cameraZoomSpeed;

	core::ActionButton _zoomIn;
	core::ActionButton _zoomOut;

	float _fieldOfView = 60.0f;
	float _targetDistance = 28.0f;
	glm::vec3 _cameraPosition {1.0f, 0.4f, 1.0f};
	float _pendingPitch = 0.0f;
	float _pendingTurn = 0.0f;
	float _pendingSpeed = -1.0f;
	const int _keyBindingContext;

	void zoom(float level);
	void toggleCameraType();
	void setCameraFirstPerson();
	void setCameraTarget();
public:
	PlayerCamera(const voxelworld::WorldMgrPtr &world, voxelrender::WorldRenderer &worldRenderer, int keyBindingContext = core::BindingContext::World) :
			_worldMgr(world), _worldRenderer(worldRenderer), _keyBindingContext(keyBindingContext) {
	}

	void setTarget(const glm::vec3& position);
	void setTargetDistance(float targetDistance);

	void construct();
	bool init(const glm::ivec2& position, const glm::ivec2& frameBufferSize, const glm::ivec2& windowSize);
	void shutdown();
	void update(const glm::vec3& entityPosition, float deltaFrameSeconds, uint64_t now, float speed);

	void setFieldOfView(float fieldOfView);
	void rotate(float pitch, float turn, float speed);

	const video::Camera& camera() const;
};

inline void PlayerCamera::setTargetDistance(float targetDistance) {
	_targetDistance = targetDistance;
}

inline void PlayerCamera::setTarget(const glm::vec3& position) {
	_camera.setTarget(position);
}

inline void PlayerCamera::setFieldOfView(float fieldOfView) {
	_camera.setFieldOfView(fieldOfView);
}

inline const video::Camera& PlayerCamera::camera() const {
	return _camera;
}

}
