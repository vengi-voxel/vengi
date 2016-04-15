#include "Camera.h"
#include "core/Common.h"
#include "core/Singleton.h"
#include "io/EventHandler.h"

namespace video {

Camera::Camera() :
		_pos(0.0f, 0.0f, 0.0f), _width(0), _height(0), _pitch(-M_PI_2), _yaw(M_PI), _direction(0.0f, 0.0f, 0.0f), _maxpitch(core::Var::get(cfg::ClientCameraMaxPitch, std::to_string(glm::radians(89.0)))) {
	updateDirection();
}

Camera::~Camera() {
}

void Camera::updatePosition(long dt, bool left, bool right, bool forward, bool backward, float speed) {
	const float angle = _yaw - M_PI_2;
	const glm::vec3 rightvec(glm::sin(angle), 0.0, glm::cos(angle));

	const float deltaTime = static_cast<float>(dt);
	if (forward) {
		_pos += _direction * deltaTime * speed;
	}
	if (backward) {
		_pos -= _direction * deltaTime * speed;
	}
	if (left) {
		_pos -= rightvec * deltaTime * speed;
	}
	if (right) {
		_pos += rightvec * deltaTime * speed;
	}
}

void Camera::init(int width, int height) {
	_width = width;
	_height = height;
}

void Camera::updateDirection() {
	const float maxPitch = _maxpitch->floatVal();
	_pitch = glm::clamp(_pitch, -maxPitch, maxPitch);

	const float cosV = glm::cos(_pitch);
	const float cosH = glm::cos(_yaw);
	const float sinH = glm::sin(_yaw);
	const float sinV = glm::sin(_pitch);
	_direction = glm::vec3(cosV * sinH, sinV, cosV * cosH);
}

void Camera::onMotion(int32_t x, int32_t y, int32_t deltaX, int32_t deltaY, float rotationSpeed) {
	_yaw -= static_cast<float>(deltaX) * rotationSpeed;
	_pitch -= static_cast<float>(deltaY) * rotationSpeed;

	updateDirection();
}

}
