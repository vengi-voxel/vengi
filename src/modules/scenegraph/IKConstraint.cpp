/**
 * @file
 */

#include "IKConstraint.h"
#include "scenegraph/SceneGraphNode.h"

namespace scenegraph {

IKConstraint::IKConstraint()
	: effectorNodeId(InvalidNodeId), rollMin(-glm::pi<float>()), rollMax(glm::pi<float>()), visible(true),
	  anchor(false) {
}

} // namespace scenegraph
