/**
 * @file
 */

#include "CoordinateSystemUtil.h"
#include "math/CoordinateSystemUtil.h"

namespace scenegraph {

// https://stackoverflow.com/a/71168853/774082
bool convertCoordinateSystem(math::CoordinateSystem from, math::CoordinateSystem to,
							 scenegraph::SceneGraph &sceneGraph) {
	glm::mat4x4 transformationMatrix1;
	glm::mat4x4 transformationMatrix2;
	if (!math::coordinateSystemTransformationMatrix(from, to, transformationMatrix1, transformationMatrix2)) {
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

SceneGraphTransform convertCoordinateSystem(math::CoordinateSystem from, const SceneGraphTransform &fromTransform) {
	SceneGraphTransform transform;
	transform.setLocalMatrix(math::convertCoordinateSystem(from, fromTransform.calculateLocalMatrix()));
	return transform;
}

} // namespace scenegraph
