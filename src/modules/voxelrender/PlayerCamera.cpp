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

void PlayerCamera::rotate(float pitch, float turn, float speed) {
	_pendingPitch = pitch;
	_pendingTurn = turn;
	if (glm::abs(pitch + turn) > glm::epsilon<float>()) {
		_pendingSpeed = speed;
	}
}

void PlayerCamera::update(const glm::vec3& entityPosition, int64_t deltaFrame) {
	// TODO: fix this magic number with the real character eye height.
	static const glm::vec3 eye(0.0f, 1.8f, 0.0f);
	const glm::vec3 targetpos = entityPosition + eye;
	_camera.setTarget(targetpos);

	if (_pendingSpeed > 0.0f) {
		// TODO: optimize this
		video::Camera clone = camera();
		const glm::vec3 radians(_pendingPitch * _pendingSpeed, _pendingTurn * _pendingSpeed, 0.0f);
		clone.rotate(radians);
		if (clone.pitch() >= glm::radians(1.0f)) {
			_camera.rotate(radians);
			_pendingSpeed = -1.0f;
		} else {
			_pendingPitch *= 0.5f;
		}
	}

	const glm::vec3& direction = _camera.direction();
	glm::vec3 hit(0.5f);
	if (_worldMgr->raycast(targetpos, direction, _targetDistance, [&] (const voxel::PagedVolume::Sampler& sampler) {
			voxel::Voxel voxel = sampler.voxel();
			if (!voxel::isEnterable(voxel.getMaterial())) {
				// store position and abort raycast
				hit += sampler.position();
				return false;
			}
			return true;
		})) {
		_camera.setTargetDistance(core_max(0.1f, glm::distance(targetpos, hit) - 0.5f));
	} else {
		_camera.setTargetDistance(_targetDistance);
	}

	_camera.setFarPlane(_worldRenderer.getViewDistance());
	_camera.update(deltaFrame);
}

}
