/**
 * @file
 */

#pragma once

#include "SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"

namespace scenegraph {

/**
 * @ingroup SceneGraph
 */
class SceneGraphNodeCamera : public SceneGraphNode {
private:
	using Super = SceneGraphNode;

public:
	SceneGraphNodeCamera(const core::UUID &uuid = core::UUID());

	static constexpr const char *Modes[] = {"orthographic", "perspective"};

	static bool isFloatProperty(const core::String &key) {
		return key == PropCamNearPlane || key == PropCamFarPlane || key == PropCamAspect;
	}

	static bool isIntProperty(const core::String &key) {
		return key == PropCamHeight || key == PropCamWidth || key == PropCamFov;
	}

	/**
	 * @brief Field of view in degree
	 */
	int fieldOfView() const;
	void setFieldOfView(int val);

	/**
	 * aspect ratio (width over height) of the field of view, or the aspect ratio of the viewport
	 */
	void setAspectRatio(float val);
	float aspectRatio() const;

	int width() const;
	void setWidth(int val);
	int height() const;
	void setHeight(int val);

	float farPlane() const;
	void setFarPlane(float val);

	float nearPlane() const;
	void setNearPlane(float val);

	bool isOrthographic() const;
	void setOrthographic();

	bool isPerspective() const;
	void setPerspective();
};
static_assert(sizeof(SceneGraphNodeCamera) == sizeof(SceneGraphNode), "Sizes must match - direct casting is performed");

inline SceneGraphNodeCamera &toCameraNode(SceneGraphNode &node) {
	core_assert(node.type() == SceneGraphNodeType::Camera);
	return (SceneGraphNodeCamera &)node;
}

inline const SceneGraphNodeCamera &toCameraNode(const SceneGraphNode &node) {
	core_assert(node.type() == SceneGraphNodeType::Camera);
	return (const SceneGraphNodeCamera &)node;
}

} // namespace scenegraph
