/**
 * @file
 */

#include "CoordinateSystemUtil.h"
#include "core/GLMConst.h"
#include "scenegraph/CoordinateSystem.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphTransform.h"

namespace scenegraph {

//
// Generate right, forward and up direction vectors to express the desired coordinate system for vengi
//
//                      Right handed   Left handed
//   -----------------------------------------------
//     Magicavoxel      OpenGL         VENGI
//     VXL              Maya           DirectX12
//     Blender
//     3dsmax
//
//     Z                Y              Y
//     | Y              |              | Z
//     |/               |              |/
//     o----X           o----X         o----X
//                     /
//                    Z
//
bool coordinateSystemToMatrix(CoordinateSystem sys, glm::mat4 &matrix) {
	// read the assignment as: our right is your ... (and so on)
	// so right = glm::right() is a no-op, but right = glm::vec3(-1, 0, 0)
	// would mean: our right is your left (with your left given in opengl
	// coordinate system)
	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 forward;
	switch (sys) {
	case CoordinateSystem::DirectX:
	case CoordinateSystem::Vengi:
		// no-op for conversion to vengi
		right = glm::right();
		up = glm::up();
		forward = glm::forward();
		break;
	case CoordinateSystem::Maya:
	case CoordinateSystem::OpenGL:
		// opengl forward is the vengi backward
		right = glm::vec3(1.0f, 0.0f, 0.0f);
		up = glm::vec3(0.0f, 1.0f, 0.0f);
		forward = glm::vec3(0.0f, 0.0f, 1.0f);
		break;
	case CoordinateSystem::Autodesk3dsmax:
	case CoordinateSystem::MagicaVoxel:
	case CoordinateSystem::VXL:
		// Z-up coordinate system (like 3dsmax).
		right = glm::vec3(1.0f, 0.0f, 0.0f);
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

	transformationMatrix1 = toSystem * fromSystem;
	transformationMatrix2 = glm::inverse(fromSystem) * glm::inverse(toSystem);
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
		core::Buffer<scenegraph::SceneGraphKeyFrame> &frames = e->value;
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
			SceneGraphKeyFrames &frames = e->value;
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

SceneGraphTransform convertCoordinateSystem(CoordinateSystem from, const SceneGraphTransform &fromTransform) {
	SceneGraphTransform transform;
	transform.setLocalMatrix(convertCoordinateSystem(from, fromTransform.calculateLocalMatrix()));
	return transform;
}

} // namespace scenegraph
