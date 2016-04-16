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

FrustumResult Camera::testFrustum(const glm::vec3& mins, const glm::vec3& maxs) const {
	FrustumResult result = FrustumResult::Inside;
	for (uint i = 0; i < MaxPlanes; i++) {
		const float pos = _frustumPlanes[i].w;
		const glm::vec3 normal(_frustumPlanes[i]);

		glm::vec3 positiveVertex = mins;
		if (normal.x >= 0.0f)
			positiveVertex.x = maxs.x;
		if (normal.y >= 0.0f)
			positiveVertex.y = maxs.y;
		if (normal.z >= 0.0f)
			positiveVertex.z = maxs.z;

		if (glm::dot(normal, positiveVertex) + pos < 0.0f) {
			return FrustumResult::Outside;
		}

		glm::vec3 negativeVertex = maxs;
		if (normal.x >= 0.0f)
			negativeVertex.x = mins.x;
		if (normal.y >= 0.0f)
			negativeVertex.y = mins.y;
		if (normal.z >= 0.0f)
			negativeVertex.z = mins.z;

		if (glm::dot(normal, negativeVertex) + pos < 0.0f) {
			result = FrustumResult::Intersect;
		}
	}

	return result;
}

void Camera::updateFrustumPlanes(const glm::mat4& p) {
	const glm::mat4 &v = _viewMatrix;

	glm::mat4 clipMatrix;

	clipMatrix[0][0] = v[0][0] * p[0][0] + v[0][1] * p[1][0] + v[0][2] * p[2][0] + v[0][3] * p[3][0];
	clipMatrix[1][0] = v[0][0] * p[0][1] + v[0][1] * p[1][1] + v[0][2] * p[2][1] + v[0][3] * p[3][1];
	clipMatrix[2][0] = v[0][0] * p[0][2] + v[0][1] * p[1][2] + v[0][2] * p[2][2] + v[0][3] * p[3][2];
	clipMatrix[3][0] = v[0][0] * p[0][3] + v[0][1] * p[1][3] + v[0][2] * p[2][3] + v[0][3] * p[3][3];
	clipMatrix[0][1] = v[1][0] * p[0][0] + v[1][1] * p[1][0] + v[1][2] * p[2][0] + v[1][3] * p[3][0];
	clipMatrix[1][1] = v[1][0] * p[0][1] + v[1][1] * p[1][1] + v[1][2] * p[2][1] + v[1][3] * p[3][1];
	clipMatrix[2][1] = v[1][0] * p[0][2] + v[1][1] * p[1][2] + v[1][2] * p[2][2] + v[1][3] * p[3][2];
	clipMatrix[3][1] = v[1][0] * p[0][3] + v[1][1] * p[1][3] + v[1][2] * p[2][3] + v[1][3] * p[3][3];
	clipMatrix[0][2] = v[2][0] * p[0][0] + v[2][1] * p[1][0] + v[2][2] * p[2][0] + v[2][3] * p[3][0];
	clipMatrix[1][2] = v[2][0] * p[0][1] + v[2][1] * p[1][1] + v[2][2] * p[2][1] + v[2][3] * p[3][1];
	clipMatrix[2][2] = v[2][0] * p[0][2] + v[2][1] * p[1][2] + v[2][2] * p[2][2] + v[2][3] * p[3][2];
	clipMatrix[3][2] = v[2][0] * p[0][3] + v[2][1] * p[1][3] + v[2][2] * p[2][3] + v[2][3] * p[3][3];
	clipMatrix[0][3] = v[3][0] * p[0][0] + v[3][1] * p[1][0] + v[3][2] * p[2][0] + v[3][3] * p[3][0];
	clipMatrix[1][3] = v[3][0] * p[0][1] + v[3][1] * p[1][1] + v[3][2] * p[2][1] + v[3][3] * p[3][1];
	clipMatrix[2][3] = v[3][0] * p[0][2] + v[3][1] * p[1][2] + v[3][2] * p[2][2] + v[3][3] * p[3][2];
	clipMatrix[3][3] = v[3][0] * p[0][3] + v[3][1] * p[1][3] + v[3][2] * p[2][3] + v[3][3] * p[3][3];

	_frustumPlanes[FrustumRight].x = clipMatrix[3][0] - clipMatrix[0][0];
	_frustumPlanes[FrustumRight].y = clipMatrix[3][1] - clipMatrix[0][1];
	_frustumPlanes[FrustumRight].z = clipMatrix[3][2] - clipMatrix[0][2];
	_frustumPlanes[FrustumRight].w = clipMatrix[3][3] - clipMatrix[0][3];

	_frustumPlanes[FrustumLeft].x = clipMatrix[3][0] + clipMatrix[0][0];
	_frustumPlanes[FrustumLeft].y = clipMatrix[3][1] + clipMatrix[0][1];
	_frustumPlanes[FrustumLeft].z = clipMatrix[3][2] + clipMatrix[0][2];
	_frustumPlanes[FrustumLeft].w = clipMatrix[3][3] + clipMatrix[0][3];

	_frustumPlanes[FrustumBottom].x = clipMatrix[3][0] + clipMatrix[1][0];
	_frustumPlanes[FrustumBottom].y = clipMatrix[3][1] + clipMatrix[1][1];
	_frustumPlanes[FrustumBottom].z = clipMatrix[3][2] + clipMatrix[1][2];
	_frustumPlanes[FrustumBottom].w = clipMatrix[3][3] + clipMatrix[1][3];

	_frustumPlanes[FrustumTop].x = clipMatrix[3][0] - clipMatrix[1][0];
	_frustumPlanes[FrustumTop].y = clipMatrix[3][1] - clipMatrix[1][1];
	_frustumPlanes[FrustumTop].z = clipMatrix[3][2] - clipMatrix[1][2];
	_frustumPlanes[FrustumTop].w = clipMatrix[3][3] - clipMatrix[1][3];

	_frustumPlanes[FrustumFar].x = clipMatrix[3][0] - clipMatrix[2][0];
	_frustumPlanes[FrustumFar].y = clipMatrix[3][1] - clipMatrix[2][1];
	_frustumPlanes[FrustumFar].z = clipMatrix[3][2] - clipMatrix[2][2];
	_frustumPlanes[FrustumFar].w = clipMatrix[3][3] - clipMatrix[2][3];

	_frustumPlanes[FrustumNear].x = clipMatrix[3][0] + clipMatrix[2][0];
	_frustumPlanes[FrustumNear].y = clipMatrix[3][1] + clipMatrix[2][1];
	_frustumPlanes[FrustumNear].z = clipMatrix[3][2] + clipMatrix[2][2];
	_frustumPlanes[FrustumNear].w = clipMatrix[3][3] + clipMatrix[2][3];

	for (int i = 0; i < MaxPlanes; i++) {
		_frustumPlanes[i] = glm::normalize(_frustumPlanes[i]);
	}
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
