/**
 * @file
 */

#pragma once

#include "math/AABB.h"
#include <list>

namespace voxedit {

using Selection = math::AABB<int>;
using Selections = std::list<Selection>;

}
