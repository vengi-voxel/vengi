/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "ui/dearimgui/imgui.h"

namespace voxelui {
namespace dragdrop {

constexpr const char *PaletteIndexPayload = "palindex";
constexpr const char *RGBAPayload = IMGUI_PAYLOAD_TYPE_COLOR_4F;
constexpr const char *RGBPayload = IMGUI_PAYLOAD_TYPE_COLOR_3F;
constexpr const char *ImagePayload = "image";
constexpr const char *ModelPayload = "model";

/**
 * Publish a pointer to a long-lived ImagePtr (e.g. panel member). Do not pass temporaries.
 * ImGui only memcpy's the pointer value - never bitwise-copy SharedPtr into the payload.
 */
inline void setImagePayload(const image::ImagePtr &heldImage) {
	if (!heldImage || !heldImage->isLoaded()) {
		return;
	}
	const image::ImagePtr *ptr = &heldImage;
	ImGui::SetDragDropPayload(ImagePayload, &ptr, sizeof(ptr), ImGuiCond_Once);
}

inline image::ImagePtr acceptImagePayload() {
	const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(ImagePayload);
	if (payload == nullptr || payload->DataSize != (int)sizeof(const image::ImagePtr *)) {
		return image::ImagePtr();
	}
	const image::ImagePtr *const *pp = (const image::ImagePtr *const *)payload->Data;
	if (pp == nullptr || *pp == nullptr) {
		return image::ImagePtr();
	}
	const image::ImagePtr &image = **pp;
	if (!image || !image->isLoaded()) {
		return image::ImagePtr();
	}
	return image;
}

} // namespace dragdrop
} // namespace voxelui
