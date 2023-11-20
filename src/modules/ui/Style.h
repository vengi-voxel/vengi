/**
 * @file
 */

#pragma once

#include "core/Color.h"

namespace style {

enum StyleColor { ColorReferenceNode, ColorInactiveNode, ColorActiveNode };

const glm::vec4 &color(StyleColor color);

} // namespace style
