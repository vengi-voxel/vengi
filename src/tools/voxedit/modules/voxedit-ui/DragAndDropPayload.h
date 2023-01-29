/**
 * @file
 */

#pragma once

#include "ui/dearimgui/imgui.h"

namespace voxedit {
namespace dragdrop {

constexpr const char *SceneNodePayload = "scenegraphnode";
constexpr const char *PaletteIndexPayload = "palindex";
constexpr const char *RGBAPayload = IMGUI_PAYLOAD_TYPE_COLOR_4F;
constexpr const char *ImagePayload = "image";
constexpr const char *ModelPayload = "model";

}
}
