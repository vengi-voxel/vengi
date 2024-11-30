/**
 * @file
 */

#pragma once

#include "ui/dearimgui/imgui.h"

namespace voxelui {
namespace dragdrop {

constexpr const char *PaletteIndexPayload = "palindex";
constexpr const char *RGBAPayload = IMGUI_PAYLOAD_TYPE_COLOR_4F;
constexpr const char *RGBPayload = IMGUI_PAYLOAD_TYPE_COLOR_3F;
constexpr const char *ImagePayload = "image";
constexpr const char *ModelPayload = "model";

} // namespace dragdrop
} // namespace voxelui
