/**
 * @file
 */

#include "SceneGraphNodeCamera.h"
#include "core/StringUtil.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphTransform.h"

namespace scenegraph {

SceneGraphNodeCamera::SceneGraphNodeCamera(const core::UUID &uuid) : Super(SceneGraphNodeType::Camera, uuid) {
}

float SceneGraphNodeCamera::farPlane() const {
	return propertyf(PropCamFarPlane);
}

void SceneGraphNodeCamera::setFarPlane(float val) {
	setProperty(PropCamFarPlane, core::string::toString(val));
}

float SceneGraphNodeCamera::nearPlane() const {
	return propertyf(PropCamNearPlane);
}

void SceneGraphNodeCamera::setNearPlane(float val) {
	setProperty(PropCamNearPlane, core::string::toString(val));
}

bool SceneGraphNodeCamera::isOrthographic() const {
	return property(PropCamMode) == Modes[0];
}

void SceneGraphNodeCamera::setOrthographic() {
	setProperty(PropCamMode, Modes[0]);
}

bool SceneGraphNodeCamera::isPerspective() const {
	return property(PropCamMode) == Modes[1];
}

void SceneGraphNodeCamera::setPerspective() {
	setProperty(PropCamMode, Modes[1]);
}

int SceneGraphNodeCamera::width() const {
	return property(PropCamWidth).toInt();
}

void SceneGraphNodeCamera::setWidth(int val) {
	setProperty(PropCamWidth, core::string::toString(val));
}

int SceneGraphNodeCamera::height() const {
	return property(PropCamHeight).toInt();
}

void SceneGraphNodeCamera::setHeight(int val) {
	setProperty(PropCamHeight, core::string::toString(val));
}

int SceneGraphNodeCamera::fieldOfView() const {
	return property(PropCamFov).toInt();
}

void SceneGraphNodeCamera::setFieldOfView(int val) {
	setProperty(PropCamFov, core::string::toString(val));
}

float SceneGraphNodeCamera::aspectRatio() const {
	return property(PropCamAspect).toFloat();
}

void SceneGraphNodeCamera::setAspectRatio(float val) {
	setProperty(PropCamAspect, core::string::toString(val));
}

} // namespace scenegraph
