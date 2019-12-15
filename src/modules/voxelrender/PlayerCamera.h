/**
 * @file
 */

#pragma once

#include "video/Camera.h"
#include "voxelworld/WorldMgr.h"
#include "WorldRenderer.h"

namespace voxelrender {

class PlayerCamera {
private:
	video::Camera _camera;
	voxelworld::WorldMgrPtr _worldMgr;
	WorldRenderer& _worldRenderer;

	float _fieldOfView = 60.0f;
	float _targetDistance = 28.0f;
	glm::vec3 _cameraPosition {1.0f, 0.4f, 1.0f};
public:
	PlayerCamera(const voxelworld::WorldMgrPtr &world, voxelrender::WorldRenderer &worldRenderer) :
			_worldMgr(world), _worldRenderer(worldRenderer) {
	}

	void setTarget(const glm::vec3& position);

	bool init(const glm::ivec2& position, const glm::ivec2& frameBufferSize, const glm::ivec2& windowSize);
	void update(int64_t deltaFrame);

	video::Camera& camera();
	const video::Camera& camera() const;
};

inline void PlayerCamera::setTarget(const glm::vec3& position) {
	_camera.setTarget(position);
}

inline video::Camera& PlayerCamera::camera() {
	return _camera;
}

inline const video::Camera& PlayerCamera::camera() const {
	return _camera;
}

}
