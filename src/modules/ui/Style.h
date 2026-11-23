/**
 * @file
 */

#pragma once

#include "color/Color.h"
#include "math/Vec4f.h"

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
	ColorBrushGizmoLine,
	ColorBrushGizmoPolygon,
	ColorAddNodeFaceHighlight,
	ColorAddNodePreview,
};

const math::Vec4f &color(StyleColor color);

} // namespace style
