/**
 * @file
 */

#pragma once

#include "color/Color.h"

namespace style {

enum StyleColor {
	ColorLockedNode,
	ColorReferenceNode,
	ColorInactiveNode,
	ColorGroupNode,
	ColorActiveNode,
	ColorHighlightArea,
	ColorGridBorder,
	ColorBone,
	ColorUVEditor,
	ColorAxisX,
	ColorAxisY,
	ColorAxisZ,
	ColorSliceRegion,
	ColorActiveBrush,
	ColorChatSystem,
	ColorChatSender,
	ColorWarningText,
};

const glm::vec4 &color(StyleColor color);

} // namespace style
