/**
 * @file
 */

#include "CoordinateSystemUtil.h"
#include "scenegraph/SceneGraph.h"
#include "math/CoordinateSystemUtil.h"
#include "math/Quatf.h"
#include "math/Mat4f.h"
#include "math/Vec3f.h"

namespace scenegraph {

// https://stackoverflow.com/a/71168853/774082
bool convertCoordinateSystem(math::CoordinateSystem from, math::CoordinateSystem to,
							 scenegraph::SceneGraph &sceneGraph) {
	math::Mat4f transformationMatrix1;
	math::Mat4f transformationMatrix2;
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
			const math::Mat4f fromWorldMatrix = frame.transform().worldMatrix();
			const math::Mat4f toWorldMatrix = fromWorldMatrix * transformationMatrix;
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
				const math::Mat4f fromLocalMatrix = frame.transform().localMatrix();
				const math::Mat4f toLocalMatrix = transformationMatrix1 * fromLocalMatrix * transformationMatrix2;
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

math::Vec3f convertUnityToVengi(const math::Vec3f &unityPos) {
	return math::Vec3f(unityPos.x, unityPos.y, -unityPos.z);
}

math::Quatf convertUnityToVengi(const math::Quatf &unityRot) {
	return math::Quatf::wxyz(-unityRot.w, unityRot.x, unityRot.y, -unityRot.z);
}

} // namespace scenegraph
