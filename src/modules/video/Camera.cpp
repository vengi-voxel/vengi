/**
 * @file
 */

#include "Camera.h"
#include "Renderer.h"
#include "core/Common.h"
#include "core/Assert.h"
#include "core/Var.h"
#include "math/AABB.h"
#include "math/Ray.h"
#include "math/Quatf.h"
#include "math/Mat4f.h"
#include "math/Vec4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Vec2i.h"
#include "math/Vec2f.h"
#include "math/Conversions.h"
#include "math/Mat4Ops.h"
#include "math/QuatOps.h"
#include "math/Axes.h"
#include "math/Scalar.h"

namespace video {

namespace {

static float quatHorizontalYaw(const math::Quatf &quat) {
	const math::Vec3f backward = -(math::conjugate(quat) * math::forward());
	const math::Vec2f projected(backward.x, backward.z);
	if (math::length2(projected) <= math::epsilonf) {
		return 0.0f;
	}
	return math::atan(projected.x, projected.y);
}

static math::Quatf isometricQuat(float yaw) {
	return math::normalize(math::angleAxis(Camera::IsometricPitch, math::right()) * math::angleAxis(-yaw, math::up()));
}

} // namespace

Camera::Camera(CameraType type, CameraMode mode) :
	_type(type), _mode(mode), _quat(math::Quatf::identity()) {
}

float Camera::pitch() const {
	return math::pitch(_quat);
}

float Camera::roll() const {
	return math::roll(_quat);
}

float Camera::yaw() const {
	return math::yaw(_quat);
}

void Camera::setYaw(float radians) {
	rotate(radians, math::up());
}

void Camera::setRoll(float radians) {
	rotate(radians, math::backward());
}

bool Camera::lookAt(const math::Vec3f& position) {
	return lookAt(position, math::up());
}

void Camera::turn(float radians) {
	if (fabs((double)radians) < 0.00001) {
		return;
	}
	if (isIsometric() && _rotationType == CameraRotationType::Target) {
		const math::Vec3f offset = _worldPos - _target;
		const math::Vec3f rotatedOffset = math::angleAxis(radians, math::up()) * offset;
		_worldPos = _target + rotatedOffset;
		_quat = isometricQuat(math::atan(rotatedOffset.x, rotatedOffset.z));
		_dirty |= DIRTY_POSITION | DIRTY_ORIENTATION;
		_lerp = false;
		return;
	}
	if (isIsometric()) {
		setOrientation(isometricQuat(quatHorizontalYaw(_quat) + radians));
		return;
	}
	const math::Vec3f axis = _quat * math::up();
	const math::Quatf& quat = math::angleAxis(radians, axis);
	rotate(quat);
}

void Camera::rotate(float radians, const math::Vec3f& axis) {
	if (fabs((double)radians) < 0.00001) {
		return;
	}
	const math::Quatf& quat = math::angleAxis(radians, axis);
	rotate(quat);
}

float Camera::worldUnitsPerPixel() const {
	if (_type == CameraType::UI)
		return 1.0f;
	if (isOrthographic())
		return _orthoZoom / PIXELS_PER_UNIT;
	const float height = (float)core_max(1, _windowSize.y);
	return 2.0f * _distance * math::tan(math::radians(_fieldOfView) * 0.5f) / height;
}

void Camera::pan(int screenDeltaX, int screenDeltaY) {
	const float zoomFactor = worldUnitsPerPixel();
	const math::Vec3f r = right() * ((float)-screenDeltaX) * zoomFactor;
	const math::Vec3f u = up() * ((float)screenDeltaY) * zoomFactor;
	const math::Vec3f delta = r + u;

	// Keep the navigation target on the same plane in both rotation modes.
	setTarget(_target + delta);
	if (_rotationType == CameraRotationType::Eye)
		setWorldPosition(_worldPos + delta);

	_lerp = false;
}

math::Vec3f Camera::forward() const {
	return math::conjugate(_quat) * math::forward();
}

math::Vec3f Camera::right() const {
	return math::conjugate(_quat) * math::right();
}

math::Vec3f Camera::up() const {
	return math::conjugate(_quat) * math::up();
}

bool Camera::isOrthoAligned() const {
	return _orthoAligned;
}

void Camera::setWorldPosition(const math::Vec3f& worldPos) {
	if (math::all(math::epsilonEqual(_worldPos, worldPos, 0.0001f))) {
		return;
	}
	_dirty |= DIRTY_POSITION;
	_worldPos = worldPos;
	if (_rotationType == CameraRotationType::Target) {
		lookAt(_target);
	}
}

math::Vec3f Camera::direction() const {
	return math::Vec3f(math::column(inverseViewMatrix(), 2));
}

void Camera::lerp(const Camera& targetCam) {
	if (&targetCam == this) {
		return;
	}
	_lerp = true;
	core_assert(!targetCam.dirty());
	_lerpTarget = {targetCam.rotationType(),
				   targetCam.target(),
				   targetCam.worldPosition(),
				   targetCam.quaternion(),
				   worldPosition(),
				   quaternion(),
				   target(),
				   0.0,
				   targetCam.targetDistance(),
				   targetDistance(),
				   targetCam.fieldOfView(),
				   fieldOfView(),
				   targetCam._orthoZoom,
				   _orthoZoom};
	setRotationType(targetCam.rotationType());
	setTarget(targetCam.target());
	setMode(targetCam.mode());
	setType(targetCam.type());
}

void Camera::setOmega(const math::Vec3f& omega) {
	core_assert(!math::any(math::isnan(omega)));
	core_assert(!math::any(math::isinf(omega)));
	_omega = omega;
}

void Camera::setTarget(const math::Vec3f& target) {
	core_assert(!math::any(math::isnan(target)));
	core_assert(!math::any(math::isinf(target)));
	if (math::all(math::epsilonEqual(_target, target, 0.0001f))) {
		return;
	}
	_dirty |= DIRTY_TARGET;
	_target = target;
}

void Camera::setAngles(float pitch, float yaw, float roll = 0.0f) {
	_quat = math::Quatf(math::Vec3f(pitch, yaw, roll));
	core_assert(!math::any(math::isnan(_quat)));
	core_assert(!math::any(math::isinf(_quat)));
	_dirty |= DIRTY_ORIENTATION;
	_lerp = false;
}

void Camera::setOrientation(const math::Quatf& quat) {
	_quat = quat;
	core_assert(!math::any(math::isnan(_quat)));
	core_assert(!math::any(math::isinf(_quat)));
	_dirty |= DIRTY_ORIENTATION;
	_lerp = false;
}

void Camera::rotate(const math::Quatf& rotation) {
	core_assert(!math::any(math::isnan(rotation)));
	core_assert(!math::any(math::isinf(rotation)));
	_quat = math::normalize(rotation * _quat);
	_dirty |= DIRTY_ORIENTATION;
	_lerp = false;
}

void Camera::setSize(const math::Vec2i& windowSize) {
	if (_windowSize == windowSize) {
		return;
	}
	_windowSize = windowSize;
	_dirty |= DIRTY_PERSPECTIVE;
}

bool Camera::move(const math::Vec3f& delta) {
	if (math::all(math::epsilonEqual(delta, math::Vec3f(0.0f), 0.0001f))) {
		return false;
	}
	_dirty |= DIRTY_POSITION;
	math::Vec3f movement = forward() * -delta.z + right() * delta.x + up() * delta.y;
	_worldPos += movement;
	if (_rotationType == CameraRotationType::Target) {
		setTarget(_target + movement);
	}
	_lerp = false;
	return true;
}

void Camera::rotate(const math::Vec3f& radians) {
	switch(_type) {
	case CameraType::FirstPerson: {
		turn(radians.y);
		setPitch(radians.x);
		break;
	}
	case CameraType::Free:
		setYaw(radians.y);
		setPitch(radians.x);
		setRoll(radians.z);
		break;
	case CameraType::UI:
	case CameraType::Max:
		break;
	}
}

float Camera::horizontalYaw() const {
	return quatHorizontalYaw(_quat);
}

void Camera::setPitch(float radians) {
	if (_type == CameraType::FirstPerson) {
		const float dotResult = math::dot(direction(), math::down());
		float curPitch = math::acos(dotResult) - math::halfPi;
		const float maxPitch = math::halfPi - 0.1f;
		if (curPitch > maxPitch) {
			curPitch = maxPitch;
		}
		if (math::abs(curPitch + radians) > maxPitch) {
			radians = (float)copysign((double)maxPitch, (double)curPitch) - curPitch;
		}
	}
	if (radians != 0) {
		rotate(radians, math::right());
	}
}

void Camera::slerp(const math::Quatf& quat, float factor) {
	core_assert(!math::any(math::isnan(quat)));
	core_assert(!math::any(math::isinf(quat)));
	// Ensure shortest-path interpolation: if dot < 0, negate target quaternion
	math::Quatf targetQuat = quat;
	if (math::dot(_quat, targetQuat) < 0.0f) {
		targetQuat = -targetQuat;
	}
	_quat = math::mix(_quat, targetQuat, factor);
	_dirty |= DIRTY_ORIENTATION;
	core_assert_msg(!math::any(math::isnan(_quat)), "Factor: %f", factor);
	core_assert_msg(!math::any(math::isinf(_quat)), "Factor: %f", factor);
}

void Camera::slerp(const math::Vec3f& radians, float factor) {
	const math::Quatf quat2(radians);
	slerp(quat2, factor);
}

bool Camera::lookAt(const math::Vec3f& position, const math::Vec3f& upDirection) {
	if (math::all(math::epsilonEqual(_worldPos, position, 0.0001f))) {
		return false;
	}

	math::Vec3f targetDir = math::normalize(position - _worldPos);
	if (math::length2(targetDir) == 0) {
		targetDir = math::forward();
	}

	math::Vec3f upDir;
	if (math::epsilonEqual(math::length2(upDirection), 0.0f, math::epsilonf)) {
		upDir = math::up();
	} else {
		upDir = upDirection;
	}

	if (math::epsilonEqual(math::length2(math::cross(upDir, targetDir)), 0.0f, math::epsilonf)) {
		upDir = math::cross(targetDir, math::right());
		if (math::epsilonEqual(math::length2(upDir), 0.0f, math::epsilonf)) {
			upDir = math::cross(targetDir, math::backward());
		}
	}

	_quat = math::quat_cast(math::lookAt(_worldPos, _worldPos + targetDir, upDir));
	_dirty |= DIRTY_ORIENTATION;
	core_assert_msg(!math::any(math::isnan(_quat)), "upDirection(%f:%f:%f), position(%f:%f:%f), _pos(%f:%f:%f)",
			upDirection.x, upDirection.y, upDirection.z, position.x, position.y, position.z, _worldPos.x, _worldPos.y, _worldPos.z);
	core_assert_msg(!math::any(math::isinf(_quat)), "upDirection(%f:%f:%f), position(%f:%f:%f), _pos(%f:%f:%f)",
			upDirection.x, upDirection.y, upDirection.z, position.x, position.y, position.z, _worldPos.x, _worldPos.y, _worldPos.z);
	return true;
}

void Camera::billboard(math::Vec3f *right, math::Vec3f *up) const {
	const math::Mat4f& view = viewMatrix();
	if (right != nullptr) {
		*right = math::Vec3f(math::row(view, 0));
	}
	if (up != nullptr) {
		*up = math::Vec3f(math::row(view, 1));
	}
}

void Camera::updateTarget() {
	if (_rotationType != CameraRotationType::Target) {
		return;
	}
	if (!isDirty(DIRTY_ORIENTATION | DIRTY_TARGET)) {
		return;
	}
	const math::Vec3f& backward = -forward();
	const math::Vec3f& newPosition = _target + backward * _distance;
	if (math::all(math::epsilonEqual(_worldPos, newPosition, 0.0001f))) {
		return;
	}
	_worldPos = newPosition;
	_dirty |= DIRTY_POSITION;
}

void Camera::resetZoom() {
	_orthoZoom = 1.0f;
	_distance = 100.0f;
	_dirty |= DIRTY_PERSPECTIVE;
	_dirty |= DIRTY_TARGET;
}

void Camera::zoom(float value) {
	const float previousDistance = _distance;
	float *target;
	if (isOrthographic()) {
		target = &_orthoZoom;
		_dirty |= DIRTY_PERSPECTIVE;
	} else {
		target = &_distance;
		_dirty |= DIRTY_TARGET;
	}

	const float speed = core::getVar(cfg::ClientCameraZoomSpeed)->floatVal();
	*target *= math::exp(speed * value);

	const float maxZoom = core::getVar(cfg::ClientCameraMaxZoom)->floatVal();
	const float minZoom = core::getVar(cfg::ClientCameraMinZoom)->floatVal();
	*target = math::clamp(*target, minZoom, maxZoom);
	if (!isOrthographic() && _rotationType == CameraRotationType::Eye) {
		setWorldPosition(_worldPos + forward() * (previousDistance - _distance));
	}
}

void Camera::updateLerp(double deltaFrameSeconds) {
	_lerpTarget.seconds += deltaFrameSeconds;
	const float t = math::clamp(0.0, 1.0, _lerpTarget.seconds);
	math::Quatf targetQuat = _lerpTarget.quat;
	if (math::dot(_lerpTarget.fromQuat, targetQuat) < 0.0f) {
		targetQuat = -targetQuat;
	}
	_quat = math::mix(_lerpTarget.fromQuat, targetQuat, t);
	_dirty |= DIRTY_ORIENTATION;
	if (_lerpTarget.rotationType == CameraRotationType::Target) {
		_target = math::mix(_lerpTarget.fromTarget, _lerpTarget.target, t);
		_distance = math::mix(_lerpTarget.fromDistance, _lerpTarget.distance, t);
		_dirty |= DIRTY_TARGET;
	} else {
		_worldPos = math::mix(_lerpTarget.fromWorldPos, _lerpTarget.worldPos, t);
	}
	_fieldOfView = math::mix(_lerpTarget.fromFieldOfView, _lerpTarget.fieldOfView, t);
	_orthoZoom = math::mix(_lerpTarget.fromOrthoZoom, _lerpTarget.orthoZoom, t);
	_dirty |= DIRTY_POSITION | DIRTY_PERSPECTIVE;
	if (_lerpTarget.seconds > 1.0) {
		_lerp = false;
	}
}

void Camera::update(double deltaFrameSeconds) {
	if (deltaFrameSeconds > 0.0) {
		if (_lerp) {
			updateLerp(deltaFrameSeconds);
		} else {
			rotate(_omega * (float)deltaFrameSeconds);
		}
	}
	if (_mode == CameraMode::Isometric && isDirty(DIRTY_ORIENTATION)) {
		const math::Quatf isoQuat = isometricQuat(quatHorizontalYaw(_quat));
		if (math::abs(math::dot(_quat, isoQuat)) < 0.99999f) {
			_quat = isoQuat;
			_dirty |= DIRTY_ORIENTATION;
		}
	}
	updateTarget();
	updateOrientation();
	updateViewMatrix();
	updateProjectionMatrix();
	updateFrustumPlanes();
	updateFrustumVertices();
	_viewProjectionMatrix = projectionMatrix() * viewMatrix();
	_dirty = 0u;
}

void Camera::updateOrientation() {
	if (!isDirty(DIRTY_ORIENTATION)) {
		return;
	}

	_quat = math::normalize(_quat);
	_orientation = math::mat4_cast(_quat);
	if (isOrthographic()) {
		const math::Vec3i &angles = math::abs(math::degrees(math::eulerAngles(_quat)));
		const math::Vec3i r(angles.x % 90, angles.y % 90, angles.z % 90);
		if ((r.x < 1 || r.x >= 89) && (r.y < 1 || r.y >= 89) && (r.z < 1 || r.z >= 89)) {
			_orthoAligned = true;
		} else {
			_orthoAligned = false;
		}
	} else {
		_orthoAligned = false;
	}
}

void Camera::updateProjectionMatrix() {
	if (!isDirty(DIRTY_PERSPECTIVE)) {
		return;
	}
	switch(_mode) {
	case CameraMode::Orthogonal:
	case CameraMode::Isometric:
		_projectionMatrix = orthogonalMatrix(nearPlane(), farPlane());
		break;
	case CameraMode::Perspective:
		_projectionMatrix = perspectiveMatrix(nearPlane(), farPlane());
		break;
	case CameraMode::Max:
		break;
	}
	_invProjectionMatrix = math::inverse(_projectionMatrix);
}

void Camera::updateViewMatrix() {
	if (!isDirty(DIRTY_ORIENTATION | DIRTY_POSITION)) {
		return;
	}
	_viewMatrix = math::translate(orientation(), -_worldPos);
	_invViewMatrix = math::inverse(_viewMatrix);
}

bool Camera::mouseRay(const math::Vec4f &view, const math::Vec2f &point, math::Ray &ray) const {
	if (math::any(math::isnan(view)) || math::any(math::isinf(view)) || math::any(math::isnan(point)) ||
		math::any(math::isinf(point)) || view.z <= 0 || view.w <= 0 || _windowSize.x <= 0 || _windowSize.y <= 0) {
		return false;
	}
	const math::Vec2f relative = point - math::Vec2f(view);
	if (relative.x < 0 || relative.y < 0 || relative.x >= view.z || relative.y >= view.w) {
		return false;
	}
	const math::Vec2f pixel = relative / math::Vec2f(view.z, view.w) * math::Vec2f(_windowSize);
	const math::Vec2i position(math::min(math::floor(pixel), math::Vec2f(_windowSize - math::Vec2i(1))));
	const math::Ray candidate = mouseRay(position);
	if (!candidate.isValid()) {
		return false;
	}
	ray = candidate;
	return true;
}

math::Ray Camera::mouseRay(const math::Vec2i &pixelPos) const {
	const math::Vec4f viewport(0.0f, 0.0f, _windowSize.x, _windowSize.y);

	// UI pixelPos is top-left; unProject / NDC math expect lower-left window coords.
	// glClipControl(UPPER_LEFT) only changes the GPU viewport mapping - always flip Y here.
	const float winX = (float)pixelPos.x;
	const float winY = (float)(_windowSize.y - pixelPos.y);

	if (isOrthographic()) {
		// For orthogonal projection, manually compute the ray origin
		// Map screen coordinates to normalized device coordinates [-1, 1]
		const float ndcX = (2.0f * winX) / _windowSize.x - 1.0f;
		const float ndcY = (2.0f * winY) / _windowSize.y - 1.0f;

		// Apply zoom and transform to view space
		const float zoom = _orthoZoom / PIXELS_PER_UNIT;
		const float halfWidth = (float)_windowSize.x / 2.0f;
		const float halfHeight = (float)_windowSize.y / 2.0f;
		const math::Vec3f viewPos(ndcX * halfWidth * zoom, ndcY * halfHeight * zoom, 0.0f);

		// Transform to world space
		const math::Vec3f origin = math::Vec3f(_invViewMatrix * math::Vec4f(viewPos, 1.0f));

		// Ortho: constant direction
		return math::Ray(origin, forward());
	}

	// Perspective: unproject near and far points
	if (clipDepthZeroToOne()) {
		const math::Vec3f origin =
			math::unProjectZO(math::Vec3f(winX, winY, 0.0f), _viewMatrix, _projectionMatrix, viewport);
		const math::Vec3f farPoint =
			math::unProjectZO(math::Vec3f(winX, winY, 1.0f), _viewMatrix, _projectionMatrix, viewport);
		return math::Ray(origin, math::normalize(farPoint - origin));
	}
	const math::Vec3f origin = math::unProject(math::Vec3f(winX, winY, 0.0f), _viewMatrix, _projectionMatrix, viewport);
	const math::Vec3f farPoint = math::unProject(math::Vec3f(winX, winY, 1.0f), _viewMatrix, _projectionMatrix, viewport);

	return math::Ray(origin, math::normalize(farPoint - origin));
}

bool Camera::worldSegmentToScreen(const math::Vec3f &from, const math::Vec3f &to, math::Vec2i &screenFrom,
								  math::Vec2i &screenTo) const {
	math::Vec4f a = viewProjectionMatrix() * math::Vec4f(from, 1);
	math::Vec4f b = viewProjectionMatrix() * math::Vec4f(to, 1);
	if (math::any(math::isinf(a)) || math::any(math::isinf(b)) || math::any(math::isnan(a)) || math::any(math::isnan(b)))
		return false;
	const math::Vec4f planes[] = {
		{1.0f, 0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f, 1.0f},
		{0.0f, 0.0f, 1.0f, clipDepthZeroToOne() ? 0.0f : 1.0f}, {0.0f, 0.0f, -1.0f, 1.0f}};
	for (const auto &plane : planes) {
		const float da = math::dot(plane, a), db = math::dot(plane, b);
		if (da < 0 && db < 0)
			return false;
		if ((da < 0) != (db < 0)) {
			const math::Vec4f intersection = math::mix(a, b, da / (da - db));
			if (da < 0)
				a = intersection;
			else
				b = intersection;
		}
	}
	if (a.w <= 0.000001f || b.w <= 0.000001f)
		return false;
	const auto project = [&](const math::Vec4f &point) {
		const math::Vec2f ndc = math::Vec2f(point) / point.w;
		return math::clamp(math::Vec2i((ndc.x * 0.5f + 0.5f) * _windowSize.x, (0.5f - ndc.y * 0.5f) * _windowSize.y),
						  math::Vec2i(0), _windowSize);
	};
	screenFrom = project(a);
	screenTo = project(b);
	return true;
}

math::Vec2i Camera::worldToScreen(const math::Vec3f& worldPos) const {
	return worldToScreen(viewProjectionMatrix(), worldPos);
}

math::Vec2i Camera::worldToScreen(const math::Mat4f& matrix, const math::Vec3f& worldPos) const {
	math::Vec4f trans = matrix * math::Vec4f(worldPos, 1.0f);
	trans *= 0.5f / trans.w;
	trans += math::Vec4f(0.5f, 0.5f, 0.0f, 0.0f);
	// NDC is y-up (bottom=0); convert to UI top-left. Independent of clip origin.
	trans.y = 1.0f - trans.y;
	trans.x *= (float)_windowSize.x;
	trans.y *= (float)_windowSize.y;
	return math::Vec2i(trans.x, trans.y);
}

/**
 * http://developer.download.nvidia.com/SDK/10.5/opengl/src/cascaded_shadow_maps/doc/cascaded_shadow_maps.pdf
 */
void Camera::sliceFrustum(float* sliceBuf, int bufSize, int splits, float sliceWeight) const {
	core_assert_always(bufSize >= splits * 2);
	core_assert_always(splits >= 1);
	// Ortho nearPlane() is clip -orthoDepth/2 (behind the camera). Cascade distances
	// are positive view-space depth, matching shadow() viewz = dot(world - eye, forward).
	const float near = math::max(_nearPlane, 0.01f);
	const float far = math::max(farPlane(), near + 1.0f);
	const float ratio = far / (math::abs(near) + math::epsilonf);
#if 1
	const int numSlices = splits * 2;
	const float splitsf = (float)numSlices;
	const float dist = far - near;
	const float sliceWeightInv = 1.0f - sliceWeight;

	sliceBuf[0] = near;
	sliceBuf[numSlices - 1] = far;

	for (int nearIdx = 2, farIdx = 1; nearIdx < numSlices; farIdx += 2, nearIdx += 2) {
		const float si = (float)farIdx / splitsf;
		const float nearp = sliceWeight * (near * math::pow(ratio, si)) + sliceWeightInv * (near + dist * si);
		sliceBuf[nearIdx] = nearp;
		sliceBuf[farIdx] = nearp * 1.005f;
	}
#else
	int bufIdx = 0;
	for (int split = 0; split < splits; ++split) {
		const float nearK = float(bufIdx) / splits;
		const float nearLogd = near * math::pow(ratio, nearK);
		const float nearLind = math::mix(near, far, nearK);
		const float nearSplitVal = math::mix(nearLogd, nearLind, sliceWeight);
		sliceBuf[bufIdx++] = nearSplitVal;

		const float farK = float(bufIdx) / splits;
		const float farLogd = near * math::pow(ratio, farK);
		const float farLind = math::mix(near, far, farK);
		const float farSplitVal = math::mix(farLogd, farLind, sliceWeight);
		sliceBuf[bufIdx++] = farSplitVal;
	}
#endif
}

void Camera::splitFrustum(float nearPlane, float farPlane, math::Vec3f out[math::FRUSTUM_VERTICES_MAX]) const {
	math::Mat4f proj;
	switch(_mode) {
	case CameraMode::Orthogonal:
	case CameraMode::Isometric:
		proj = orthogonalMatrix(nearPlane, farPlane);
		break;
	case CameraMode::Perspective:
	default:
		proj = perspectiveMatrix(nearPlane, farPlane);
		break;
	}

	const math::Mat4f& transform = math::inverse(proj * viewMatrix());
	frustum().split(transform, out, clipDepthZeroToOne());
}

void Camera::updateFrustumVertices() {
	if (!isDirty(DIRTY_ORIENTATION | DIRTY_POSITION | DIRTY_PERSPECTIVE)) {
		return;
	}

	_frustum.updateVertices(viewMatrix(), projectionMatrix(), clipDepthZeroToOne());
}

void Camera::updateFrustumPlanes() {
	if (!isDirty(DIRTY_ORIENTATION | DIRTY_POSITION | DIRTY_PERSPECTIVE)) {
		return;
	}

	_frustum.updatePlanes(viewMatrix(), projectionMatrix(), clipDepthZeroToOne());
}

math::AABB<float> Camera::aabb() const {
	core_assert(!isDirty(DIRTY_ORIENTATION | DIRTY_POSITION | DIRTY_PERSPECTIVE));
	return frustum().aabb();
}

math::Vec4f Camera::sphereBoundingBox() const {
	const math::AABB<float> boundingBox = aabb();
	const math::Vec3f& mins = boundingBox.getLowerCorner();
	const math::Vec3f& maxs = boundingBox.getUpperCorner();

	const math::Vec3f sphereCenter(
			mins.x + (maxs.x - mins.x) / 2.0f,
			mins.y + (maxs.x - mins.y) / 2.0f,
			mins.z + (maxs.z - mins.z) / 2.0f);
	const float sphereRadius = core_max(core_max((maxs.x - mins.x) / 2.0f, (maxs.y - mins.y) / 2.0f), (maxs.z - mins.z) / 2.0f);

	return math::Vec4f(sphereCenter, sphereRadius);
}

math::Mat4f Camera::orthogonalMatrix(float nplane, float fplane) const {
	if (_type == CameraType::UI) {
		const float left = 0.0f;
		const float top = 0.0f;
		const float right = (float)_windowSize.x;
		const float bottom = (float)_windowSize.y;
		core_assert_msg(right > left, "Invalid dimension given: right must be greater than left but is %f", right);
		core_assert_msg(top < bottom, "Invalid dimension given: top must be smaller than bottom but is %f", top);
		if (clipDepthZeroToOne()) {
			return math::orthoZO(left, right, bottom, top, nplane, fplane);
		}
		return math::ortho(left, right, bottom, top, nplane, fplane);
	}
	const float zoom = _orthoZoom / PIXELS_PER_UNIT;
	const float halfWidth = (float)_windowSize.x / 2.0f;
	const float halfHeight = (float)_windowSize.y / 2.0f;
	const float left = -halfWidth;
	const float right = halfWidth;
	const float bottom = -halfHeight;
	const float top = halfHeight;

	if (clipDepthZeroToOne()) {
		return math::orthoRH_ZO(left * zoom, right * zoom, bottom * zoom, top * zoom, nplane, fplane);
	}
	return math::orthoRH(left * zoom, right * zoom, bottom * zoom, top * zoom, nplane, fplane);
}

math::Mat4f Camera::perspectiveMatrix(float nplane, float fplane) const {
	const float fov = math::radians(_fieldOfView);
	if (clipDepthZeroToOne()) {
		return math::perspectiveFovRH_ZO(fov, (float)_windowSize.x, (float)_windowSize.y, nplane, fplane);
	}
	return math::perspectiveFovRH(fov, (float)_windowSize.x, (float)_windowSize.y, nplane, fplane);
}

void Camera::setOrthoDepth(float depth) {
	_orthoDepth = math::max(depth, 1.0f);
	_dirty |= DIRTY_PERSPECTIVE;
}

void Camera::setNearPlane(float nearPlane) {
	if (math::epsilonEqual(_nearPlane, nearPlane, 0.00001f)) {
		return;
	}

	_dirty |= DIRTY_PERSPECTIVE;
	if (isOrthographic()) {
		_nearPlane = nearPlane;
	} else {
		_nearPlane = core_max(0.1f, nearPlane);
	}
}

void Camera::setFarPlane(float farPlane) {
	if (math::epsilonEqual(_farPlane, farPlane, 0.00001f)) {
		return;
	}

	_dirty |= DIRTY_PERSPECTIVE;
	_farPlane = farPlane;
}

bool Camera::isVisible(const math::AABB<float>& aabb) const {
	return isVisible(aabb.getLowerCorner(), aabb.getUpperCorner());
}

bool Camera::operator==(const Camera &other) const {
	return _type == other._type && _mode == other._mode && _rotationType == other._rotationType &&
		   _windowSize == other._windowSize && math::epsilonEqual(_distance, other._distance, 0.0001f) &&
		   math::epsilonEqual(_fieldOfView, other._fieldOfView, 0.0001f) &&
		   math::epsilonEqual(_orthoZoom, other._orthoZoom, 0.0001f) &&
		   math::all(math::epsilonEqual(_worldPos, other._worldPos, 0.0001f)) &&
		   math::all(math::epsilonEqual(_target, other._target, 0.0001f)) &&
		   math::all(math::epsilonEqual(_quat, other._quat, 0.0001f));
}

}
