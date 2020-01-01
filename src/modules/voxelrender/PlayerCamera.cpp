/**
 * @file
 */

#include "PlayerCamera.h"
#include "voxel/PagedVolume.h"

namespace voxelrender {

bool PlayerCamera::init(const glm::ivec2& position, const glm::ivec2& frameBufferSize, const glm::ivec2& windowSize) {
	_camera.init(position, frameBufferSize, windowSize);
	_camera.setFarPlane(10.0f);
	_camera.setRotationType(video::CameraRotationType::Target);
	_camera.setFieldOfView(_fieldOfView);
	_camera.setTargetDistance(_targetDistance);
	_camera.setPosition(_cameraPosition);
	_camera.setTarget(glm::zero<glm::vec3>());
	_camera.setAngles(0.0f, 0.0f, 0.0f);
	_camera.update(0l);

	return true;
}

void PlayerCamera::update(const glm::vec3& entityPosition, int64_t deltaFrame) {
	static const glm::vec3 eye(0.0f, 1.8f, 0.0f);
	_camera.setTarget(entityPosition + eye);
	_camera.update(deltaFrame);
	const glm::vec3& targetpos = _camera.target();
	const glm::vec3& direction = _camera.direction();
	glm::vec3 hit;

	if (_worldMgr->raycast(targetpos, direction, _targetDistance, [&] (const voxel::PagedVolume::Sampler& sampler) {
			voxel::Voxel voxel = sampler.voxel();
			if (!voxel::isEnterable(voxel.getMaterial())) {
				// store position and abort raycast
				hit = glm::vec3(sampler.position());
				return false;
			}
			return true;
		})) {
		_camera.setTargetDistance(glm::distance(targetpos, hit));
	} else {
		_camera.setTargetDistance(_targetDistance);
	}

	_camera.setFarPlane(_worldRenderer.getViewDistance());
}

}
