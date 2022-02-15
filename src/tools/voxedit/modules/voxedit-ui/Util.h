/**
 * @file
 */

#pragma once

#include "ScopedStyle.h"
#include "command/CommandHandler.h"
#include "math/Axis.h"

namespace voxedit {
namespace veui {

void AxisStyleButton(ui::imgui::ScopedStyle &style, math::Axis axis);
void AxisStyleText(ui::imgui::ScopedStyle &style, math::Axis axis, bool dark = true);
const char *AxisButton(math::Axis axis, const char *name, const char *command, const char *icon, const char *tooltip,
					   float width, command::CommandExecutionListener *listener);
bool InputAxisInt(math::Axis axis, const char *name, int* value);
bool CheckboxAxisFlags(math::Axis axis, const char *name, math::Axis* value);

} // namespace veui
} // namespace voxedit
