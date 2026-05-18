/**
 * @file
 */

#pragma once

#include "BrushPanelContext.h"
#include "math/Axis.h"

namespace command {
struct CommandExecutionListener;
}

namespace voxedit {

class AABBBrush;
class Brush;

/**
 * @brief Shared ImGui controls reused by multiple brush panel sections (mirror, AABB, clamping).
 */
namespace brushpanel {

bool mirrorAxisRadioButton(const char *title, math::Axis type, command::CommandExecutionListener &listener,
						   Brush &brush);
void addMirrorPlanes(command::CommandExecutionListener &listener, Brush &brush);
void aabbBrushOptions(command::CommandExecutionListener &listener, AABBBrush &brush);
void aabbBrushModeOptions(AABBBrush &brush);
void addBrushClampingOption(Brush &brush);

} // namespace brushpanel
} // namespace voxedit
