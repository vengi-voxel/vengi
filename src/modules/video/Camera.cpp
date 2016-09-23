/**
 * @file
 */

#include "Camera.h"
#include "core/Common.h"
#include "core/Singleton.h"
#include "io/EventHandler.h"
#include "core/GLM.h"

namespace video {

Camera::Camera(CameraType type, CameraMode mode) :
	_type(type), _mode(mode), _pos(glm::vec3()), _omega(0.0f) {
}

Camera::~Camera() {
}

void Camera::move(const glm::vec3& delta) {
	if (glm::all(glm::epsilonEqual(delta, glm::vec3(), 0.0001f))) {
		return;
	}
	_dirty |= DIRTY_POSITON;
	_pos += forward() * -delta.z;
	_pos += right() * delta.x;
	_pos += up() * delta.y;
	if (_rotationType == CameraRotationType::Target) {
		lookAt(_target, glm::up);
		_dirty |= DIRTY_TARGET;
	}
}

void Camera::rotate(const glm::vec3& radians) {
	// TODO: what about rotationtype... should matter here, too, no?
	switch(_type) {
	case CameraType::FirstPerson:
		turn(radians.y);
		pitch(radians.x);
		break;
	case CameraType::Free:
		yaw(radians.y);
		pitch(radians.x);
		roll(radians.z);
		break;
	}
	_dirty |= DIRTY_ORIENTATION;
}

inline void Camera::slerp(const glm::quat& quat, float factor) {
	_quat = glm::mix(_quat, quat, factor);
	_dirty |= DIRTY_ORIENTATION;
	core_assert(!glm::any(glm::isnan(_quat)));
	core_assert(!glm::any(glm::isinf(_quat)));
}

void Camera::slerp(const glm::vec3& radians, float factor) {
	const glm::quat quat2(radians);
	_quat = glm::mix(_quat, quat2, factor);
	_dirty |= DIRTY_ORIENTATION;
	core_assert(!glm::any(glm::isnan(_quat)));
	core_assert(!glm::any(glm::isinf(_quat)));
}

void Camera::lookAt(const glm::vec3& position, const glm::vec3& upDirection) {
	core_assert(position != _pos);
	_quat = glm::quat_cast(glm::lookAt(_pos, position, upDirection));
	_dirty |= DIRTY_ORIENTATION;
	core_assert(!glm::any(glm::isnan(_quat)));
	core_assert(!glm::any(glm::isinf(_quat)));
}

void Camera::updateTarget() {
	if (_rotationType != CameraRotationType::Target) {
		return;
	}
	if (!isDirty(DIRTY_ORIENTATION | DIRTY_TARGET)) {
		return;
	}
	const glm::vec3& backward = -forward();
	const glm::vec3& newPosition = _target + backward * _distance;
	if (glm::all(glm::epsilonEqual(_pos, newPosition, 0.0001f))) {
		return;
	}
	_pos = newPosition;
	_dirty |= DIRTY_POSITON;
}

void Camera::update(long deltaFrame) {
	const float dt = deltaFrame / 1000.0f;
	rotate(_omega * dt);
	updateTarget();
	updateOrientation();
	updateViewMatrix();
	updateProjectionMatrix();
	updateFrustumPlanes();
	updateFrustumVertices();
	_dirty = 0;
}

void Camera::updateOrientation() {
	if (!isDirty(DIRTY_ORIENTATION)) {
		return;
	}

	_quat = glm::normalize(_quat);
	_orientation = glm::mat4_cast(_quat);
}

void Camera::updateProjectionMatrix() {
	if (!isDirty(DIRTY_PERSPECTIVE)) {
		return;
	}
	switch(_mode) {
	case CameraMode::Orthogonal:
		_projectionMatrix = orthogonalMatrix();
		break;
	case CameraMode::Perspective:
		_projectionMatrix = perspectiveMatrix();
		break;
	}
}

void Camera::updateViewMatrix() {
	if (!isDirty(DIRTY_ORIENTATION | DIRTY_POSITON)) {
		return;
	}
	_viewMatrix = glm::translate(orientation(), -_pos);
}

Ray Camera::screenRay(const glm::vec2& screenPos) const {
	// project relative mouse cursor position [0.0-1.0] to [-1.0,1.0] and flip y axis
	const float x = +(screenPos.x - 0.5f) * 2.0f;
	const float y = -(screenPos.y - 0.5f) * 2.0f;
	const glm::mat4& viewProjInverse = glm::inverse(projectionMatrix() * viewMatrix());
	const glm::vec4 near(x, y, 0.0f, 1.0f);
	const glm::vec4 far(x, y, 1.0f, 1.0f);
	const glm::vec4& origin = viewProjInverse * near;
	return Ray(glm::vec3(origin), glm::normalize(glm::vec3(viewProjInverse * far - origin)));
}

glm::vec3 Camera::screenToWorld(const glm::vec3& screenPos) const {
	const Ray& ray = screenRay(glm::vec2(screenPos));
	return ray.origin + ray.direction * screenPos.z;
}

FrustumResult Camera::testFrustum(const glm::vec3& position) const {
	FrustumResult result = FrustumResult::Inside;
	for (int i = 0; i < FRUSTUM_PLANES_MAX; i++) {
		const glm::vec3 normal(_frustumPlanes[i]);
		const float pos = _frustumPlanes[i].w;
		if (glm::dot(normal, position) + pos < 0.0f) {
			return FrustumResult::Outside;
		}
	}
	return result;
}

FrustumResult Camera::testFrustum(const glm::vec3& mins, const glm::vec3& maxs) const {
	FrustumResult result = FrustumResult::Inside;
	for (uint i = 0; i < FRUSTUM_PLANES_MAX; i++) {
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

/**
 * http://developer.download.nvidia.com/SDK/10.5/opengl/src/cascaded_shadow_maps/doc/cascaded_shadow_maps.pdf
 */
void Camera::sliceFrustum(float* sliceBuf, int bufSize, int splits, float sliceWeight) const {
	core_assert_always(bufSize >= splits * 2);
	core_assert_always(splits >= 1);
	const float near = nearPlane();
	const float far = farPlane();
	const int numSlices = splits * 2;
	const float numSlicesf = float(numSlices);
	const float ratio = far / near;
	const float delta = far - near;

	sliceBuf[0] = near;
	sliceBuf[numSlices - 1] = far;

	for (int farIndex = 1, nearIndex = 2; nearIndex < numSlices; farIndex += 2, nearIndex += 2) {
		const float exponent = farIndex / numSlicesf;
		const float one = sliceWeight * (near * glm::pow(ratio, exponent));
		const float two = (1 - sliceWeight) * (near + delta * exponent);
		const float nearPlaneSlice = one + two;
		sliceBuf[nearIndex] = nearPlaneSlice;
		sliceBuf[farIndex] = nearPlaneSlice * 1.005f;
	}
}

void Camera::splitFrustum(float nearPlane, float farPlane, glm::vec3 out[FRUSTUM_VERTICES_MAX]) const {
	static const glm::vec4 vecs[video::FRUSTUM_VERTICES_MAX] = {
		glm::vec4(-1.0f,  1.0f,  1.0f, 1.0f), glm::vec4(-1.0f, -1.0f,  1.0f, 1.0f),
		glm::vec4( 1.0f,  1.0f,  1.0f, 1.0f), glm::vec4( 1.0f, -1.0f,  1.0f, 1.0f),
		glm::vec4(-1.0f,  1.0f, -1.0f, 1.0f), glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),
		glm::vec4( 1.0f,  1.0f, -1.0f, 1.0f), glm::vec4( 1.0f, -1.0f, -1.0f, 1.0f)
	};

	glm::mat4 proj(glm::uninitialize);
	switch(_mode) {
	case CameraMode::Orthogonal:
		proj = glm::ortho(0.0f, (float)width(), (float)height(), 0.0f, nearPlane, farPlane);
		break;
	case CameraMode::Perspective:
		proj = glm::perspective(glm::radians(_fieldOfView), _aspectRatio, nearPlane, farPlane);
		break;
	}

	const glm::mat4& transform = glm::inverse(proj * viewMatrix());
	for (int i = 0; i < FRUSTUM_VERTICES_MAX; ++i) {
		const glm::vec4& v = transform * vecs[i];
		out[i] = v.xyz() / v.w;
		core_assert(!glm::any(glm::isnan(out[i])));
	}
}

void Camera::updateFrustumVertices() {
	if (!isDirty(DIRTY_ORIENTATION | DIRTY_POSITON | DIRTY_PERSPECTIVE)) {
		return;
	}

	static const glm::vec4 vecs[video::FRUSTUM_VERTICES_MAX] = {
		glm::vec4(-1.0f,  1.0f,  1.0f, 1.0f), glm::vec4(-1.0f, -1.0f,  1.0f, 1.0f),
		glm::vec4( 1.0f,  1.0f,  1.0f, 1.0f), glm::vec4( 1.0f, -1.0f,  1.0f, 1.0f),
		glm::vec4(-1.0f,  1.0f, -1.0f, 1.0f), glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),
		glm::vec4( 1.0f,  1.0f, -1.0f, 1.0f), glm::vec4( 1.0f, -1.0f, -1.0f, 1.0f)
	};

	if (_mode == CameraMode::Orthogonal) {
		const glm::mat4& transform = viewMatrix() * projectionMatrix();
		for (int i = 0; i < video::FRUSTUM_VERTICES_MAX; ++i) {
			const glm::vec4& v = transform * vecs[i];
			_frustumVertices[i] = v.xyz();
			core_assert(!glm::any(glm::isnan(_frustumVertices[i])));
			core_assert(!glm::any(glm::isinf(_frustumVertices[i])));
		}
		return;
	}

	const glm::mat4& transform = glm::inverse(projectionMatrix() * viewMatrix());
	for (int i = 0; i < video::FRUSTUM_VERTICES_MAX; ++i) {
		const glm::vec4& v = transform * vecs[i];
		_frustumVertices[i] = v.xyz() / v.w;
		core_assert(!glm::any(glm::isnan(_frustumVertices[i])));
		core_assert(!glm::any(glm::isinf(_frustumVertices[i])));
	}
}

void Camera::updateFrustumPlanes() {
	if (!isDirty(DIRTY_ORIENTATION | DIRTY_POSITON | DIRTY_PERSPECTIVE)) {
		return;
	}
	const glm::mat4 &v = viewMatrix();
	const glm::mat4 &p = projectionMatrix();
	const glm::mat4 &clipMatrix = p * v;

	_frustumPlanes[int(FrustumPlanes::Right)].x = clipMatrix[3][0] - clipMatrix[0][0];
	_frustumPlanes[int(FrustumPlanes::Right)].y = clipMatrix[3][1] - clipMatrix[0][1];
	_frustumPlanes[int(FrustumPlanes::Right)].z = clipMatrix[3][2] - clipMatrix[0][2];
	_frustumPlanes[int(FrustumPlanes::Right)].w = clipMatrix[3][3] - clipMatrix[0][3];

	_frustumPlanes[int(FrustumPlanes::Left)].x = clipMatrix[3][0] + clipMatrix[0][0];
	_frustumPlanes[int(FrustumPlanes::Left)].y = clipMatrix[3][1] + clipMatrix[0][1];
	_frustumPlanes[int(FrustumPlanes::Left)].z = clipMatrix[3][2] + clipMatrix[0][2];
	_frustumPlanes[int(FrustumPlanes::Left)].w = clipMatrix[3][3] + clipMatrix[0][3];

	_frustumPlanes[int(FrustumPlanes::Bottom)].x = clipMatrix[3][0] + clipMatrix[1][0];
	_frustumPlanes[int(FrustumPlanes::Bottom)].y = clipMatrix[3][1] + clipMatrix[1][1];
	_frustumPlanes[int(FrustumPlanes::Bottom)].z = clipMatrix[3][2] + clipMatrix[1][2];
	_frustumPlanes[int(FrustumPlanes::Bottom)].w = clipMatrix[3][3] + clipMatrix[1][3];

	_frustumPlanes[int(FrustumPlanes::Top)].x = clipMatrix[3][0] - clipMatrix[1][0];
	_frustumPlanes[int(FrustumPlanes::Top)].y = clipMatrix[3][1] - clipMatrix[1][1];
	_frustumPlanes[int(FrustumPlanes::Top)].z = clipMatrix[3][2] - clipMatrix[1][2];
	_frustumPlanes[int(FrustumPlanes::Top)].w = clipMatrix[3][3] - clipMatrix[1][3];

	_frustumPlanes[int(FrustumPlanes::Far)].x = clipMatrix[3][0] - clipMatrix[2][0];
	_frustumPlanes[int(FrustumPlanes::Far)].y = clipMatrix[3][1] - clipMatrix[2][1];
	_frustumPlanes[int(FrustumPlanes::Far)].z = clipMatrix[3][2] - clipMatrix[2][2];
	_frustumPlanes[int(FrustumPlanes::Far)].w = clipMatrix[3][3] - clipMatrix[2][3];

	_frustumPlanes[int(FrustumPlanes::Near)].x = clipMatrix[3][0] + clipMatrix[2][0];
	_frustumPlanes[int(FrustumPlanes::Near)].y = clipMatrix[3][1] + clipMatrix[2][1];
	_frustumPlanes[int(FrustumPlanes::Near)].z = clipMatrix[3][2] + clipMatrix[2][2];
	_frustumPlanes[int(FrustumPlanes::Near)].w = clipMatrix[3][3] + clipMatrix[2][3];

	for (int i = 0; i < FRUSTUM_PLANES_MAX; i++) {
		_frustumPlanes[i] = glm::normalize(_frustumPlanes[i]);
	}
}

core::AABB<float> Camera::aabb() const {
	core_assert(!isDirty(DIRTY_ORIENTATION | DIRTY_POSITON | DIRTY_PERSPECTIVE));
	return core::AABB<float>::construct(_frustumVertices, FRUSTUM_VERTICES_MAX);
}

glm::vec4 Camera::sphereBoundingBox() const {
	const core::AABB<float> boundingBox = aabb();
	const glm::vec3& mins = boundingBox.getLowerCorner();
	const glm::vec3& maxs = boundingBox.getUpperCorner();

	const glm::vec3 sphereCenter(
			mins.x + (maxs.x - mins.x) / 2.0f,
			mins.y + (maxs.x - mins.y) / 2.0f,
			mins.z + (maxs.z - mins.z) / 2.0f);
	const float sphereRadius = std::max({
			(maxs.x - mins.x) / 2.0f,
			(maxs.y - mins.y) / 2.0f,
			(maxs.z - mins.z) / 2.0f});

	return glm::vec4(sphereCenter, sphereRadius);
}


}
