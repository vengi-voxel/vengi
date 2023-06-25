/**
 * @file
 */

#include "CoordinateSystemUtil.h"
#include "core/GLMConst.h"
#include "scenegraph/CoordinateSystem.h"

namespace scenegraph {

// convert from left handed coordinate system (z up) to right handed glm coordinate system (y up)
// VXL
glm::mat4 switchYAndZ(const glm::mat4 &in) {
	// clang-format off
	static const glm::mat4 mat{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f};
	// clang-format on
	return mat * in * glm::inverse(mat);
}

// Magicavoxel
glm::mat4 transformMatrix() {
	// rotation matrix to convert into our coordinate system (mv has z pointing upwards)
	// clang-format off
	const glm::mat4 zUpMat{
		-1.0f, 0.0f, 0.0f, 0.0f,
		 0.0f, 0.0f, 1.0f, 0.0f,
		 0.0f, 1.0f, 0.0f, 0.0f,
		 0.0f, 0.0f, 0.0f, 1.0f
	};
	// clang-format on
	// glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
	// glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	return zUpMat;
}

static bool coordinateSystemToMatrix(CoordinateSystem sys, glm::mat4 &matrix) {
	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 forward;
	switch (sys) {
	case CoordinateSystem::XRightYUpZBack:
		right = glm::right;
		up = glm::up;
		forward = glm::forward;
		break;
	case CoordinateSystem::XRightYForwardZUp:
		right = glm::vec3(1.0f, 0.0f, 0.0f);
		up = glm::vec3(0.0f, 0.0f, 1.0f);
		forward = glm::vec3(0.0f, 1.0f, 0.0f);
		break;
	case CoordinateSystem::XLeftYForwardZUp:
		right = glm::vec3(-1.0f, 0.0f, 0.0f);
		up = glm::vec3(0.0f, 0.0f, 1.0f);
		forward = glm::vec3(0.0f, 1.0f, 0.0f);
		break;
	case CoordinateSystem::Max:
	default:
		return false;
	}
	// clang-format off
	matrix = glm::mat4(
		glm::vec4(right, 0.0f),
		glm::vec4(up, 0.0f),
		glm::vec4(forward, 0.0f),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	);
	// clang-format on
	return true;
}

static bool coordinateSystemTransformationMatrix(CoordinateSystem from, CoordinateSystem to,
												 glm::mat4 &transformationMatrix1, glm::mat4 &transformationMatrix2) {
	if (from == to) {
		return false;
	}

	glm::mat4 fromSystem;
	if (!coordinateSystemToMatrix(from, fromSystem)) {
		return false;
	}

	glm::mat4 toSystem;
	if (!coordinateSystemToMatrix(to, toSystem)) {
		return false;
	}

	transformationMatrix1 = /*toSystem **/ fromSystem;
	transformationMatrix2 = glm::inverse(fromSystem);
	return true;
}

glm::mat4 convertCoordinateSystem(CoordinateSystem from, CoordinateSystem to, const glm::mat4 &fromMatrix) {
	glm::mat4x4 transformationMatrix1;
	glm::mat4x4 transformationMatrix2;
	if (!coordinateSystemTransformationMatrix(from, to, transformationMatrix1, transformationMatrix2)) {
		return fromMatrix;
	}
	return transformationMatrix1 * fromMatrix * transformationMatrix2;
}

// https://stackoverflow.com/a/71168853/774082
bool convertCoordinateSystem(CoordinateSystem from, CoordinateSystem to, scenegraph::SceneGraph &sceneGraph) {
	glm::mat4x4 transformationMatrix1;
	glm::mat4x4 transformationMatrix2;
	if (!coordinateSystemTransformationMatrix(from, to, transformationMatrix1, transformationMatrix2)) {
		return false;
	}

	// Update the scene graph's transforms to ensure the matrix is updated before we
	// apply the transformation matrix
	sceneGraph.updateTransforms();

#if 0
	scenegraph::SceneGraphNode &rootNode = sceneGraph.node(0);
	scenegraph::SceneGraphKeyFramesMap &allKeyFrames = rootNode.allKeyFrames();
	for (auto e : allKeyFrames) {
		core::DynamicArray<scenegraph::SceneGraphKeyFrame> &frames = e->value;
		for (scenegraph::SceneGraphKeyFrame &frame : frames) {
			// the world matrix is still in 'fromSystem' coordinates
			const glm::mat4x4 fromWorldMatrix = frame.transform().worldMatrix();
			const glm::mat4x4 toWorldMatrix = fromWorldMatrix * transformationMatrix;
			frame.transform().setWorldMatrix(toWorldMatrix);
		}
	}
#else
	for (auto iter = sceneGraph.beginAll(); iter != sceneGraph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		scenegraph::SceneGraphKeyFramesMap &allKeyFrames = node.allKeyFrames();
		for (auto e : allKeyFrames) {
			core::DynamicArray<scenegraph::SceneGraphKeyFrame> &frames = e->value;
			for (scenegraph::SceneGraphKeyFrame &frame : frames) {
				// the local matrix is still in 'fromSystem' coordinates
				const glm::mat4x4 fromLocalMatrix = frame.transform().localMatrix();
				const glm::mat4x4 toLocalMatrix = transformationMatrix1 * fromLocalMatrix * transformationMatrix2;
				frame.transform().setLocalMatrix(toLocalMatrix);
			}
		}
	}
#endif

	// Update the scene graph's transforms again after we converted the coordinate system
	sceneGraph.updateTransforms();

	return true;
}

} // namespace scenegraph
