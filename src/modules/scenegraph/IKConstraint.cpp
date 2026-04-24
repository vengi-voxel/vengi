/**
 * @file
 */

#include "IKConstraint.h"
#include "scenegraph/SceneGraphNode.h"

namespace scenegraph {

IKConstraint::IKConstraint()
	: effectorNodeId(InvalidNodeId), rollMin(0.0f), rollMax(glm::two_pi<float>()), visible(true),
	  anchor(false) {
}

} // namespace scenegraph
