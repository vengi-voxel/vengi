/**
 * @file
 */

#pragma once

#include "video/Camera.h"
#include "voxelworld/WorldMgr.h"
#include "WorldRenderer.h"

namespace voxelrender {

/**
 * @brief The player camera clips against the world while moving.
 */
class PlayerCamera {
private:
	video::Camera _camera;
	voxelworld::WorldMgrPtr _worldMgr;
	WorldRenderer& _worldRenderer;

	float _fieldOfView = 60.0f;
	float _targetDistance = 28.0f;
	glm::vec3 _cameraPosition {1.0f, 0.4f, 1.0f};
	float _pendingPitch = 0.0f;
	float _pendingTurn = 0.0f;
	float _pendingSpeed = 0.0f;
public:
	PlayerCamera(const voxelworld::WorldMgrPtr &world, voxelrender::WorldRenderer &worldRenderer) :
			_worldMgr(world), _worldRenderer(worldRenderer) {
	}

	void setTarget(const glm::vec3& position);

	bool init(const glm::ivec2& position, const glm::ivec2& frameBufferSize, const glm::ivec2& windowSize);
	void update(const glm::vec3& entityPosition, int64_t deltaFrame);

	void setFieldOfView(float fieldOfView);
	void rotate(float pitch, float turn, float speed);

	const video::Camera& camera() const;
};

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
