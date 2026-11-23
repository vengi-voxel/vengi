/**
 * @file
 */

#include "IKConstraint.h"
#include "math/Scalar.h"

namespace scenegraph {

IKConstraint::IKConstraint()
	: rollMin(0.0f), rollMax(math::twoPi), visible(true),
	  anchor(false) {
}

} // namespace scenegraph
