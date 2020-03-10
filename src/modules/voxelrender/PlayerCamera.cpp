/**
 * @file
 */

#include "PlayerCamera.h"
#include "voxel/PagedVolume.h"
#include "core/command/Command.h"

namespace voxelrender {

void PlayerCamera::construct() {
	_maxTargetDistance = core::Var::get(cfg::ClientCameraMaxTargetDistance, "28.0");
	_cameraZoomSpeed = core::Var::get(cfg::ClientCameraZoomSpeed, "10.0");

	core::Command::registerActionButton("zoom_in", _zoomIn);
	core::Command::registerActionButton("zoom_out", _zoomOut);
}

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

void PlayerCamera::shutdown() {
	core::Command::unregisterActionButton("zoom_in");
	core::Command::unregisterActionButton("zoom_out");
}

void PlayerCamera::zoom(float level) {
	const float value = _cameraZoomSpeed->floatVal() * level;
	_targetDistance = glm::clamp(_targetDistance + value, 1.0f, _maxTargetDistance->floatVal());
}

void PlayerCamera::rotate(float pitch, float turn, float speed) {
	_pendingPitch = pitch;
	_pendingTurn = turn;
	if (glm::abs(pitch + turn) > glm::epsilon<float>()) {
		_pendingSpeed = speed;
	}
}

void PlayerCamera::update(const glm::vec3& entityPosition, float deltaFrameSeconds, uint64_t now, float speed) {
	if (_zoomIn.pressed()) {
		_zoomIn.execute(now, 20ul, [&] () {
			zoom(1.0f);
		});
	} else if (_zoomOut.pressed()) {
		_zoomOut.execute(now, 20ul, [&] () {
			zoom(-1.0f);
		});
	}

	// TODO: fix this magic number with the real character eye height.
	static const glm::vec3 eye(0.0f, 1.8f, 0.0f);
	const glm::vec3& currentPos = _camera.target();
	const glm::vec3 targetpos = entityPosition + eye;
	// TODO: first time... do a time warp...
	_camera.setTarget(glm::mix(currentPos, targetpos, deltaFrameSeconds * speed));

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
	// TODO: we should not adopt the camera target distance but use th depth buffer to make those pixels transparent that
	// are blocking the sight
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
	const uint64_t deltaFrame = (uint64_t)(deltaFrameSeconds * 1000.0f);
	_camera.update(deltaFrame);
}

}
