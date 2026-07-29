/**
 * @file
 */

#include "IKConstraint.h"
#include <glm/gtc/constants.hpp>

namespace scenegraph {

IKConstraint::IKConstraint()
	: rollMin(0.0f), rollMax(glm::two_pi<float>()), visible(true),
	  anchor(false) {
}

} // namespace scenegraph
