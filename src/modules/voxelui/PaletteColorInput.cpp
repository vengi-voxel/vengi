/**
 * @file
 */

#include "PaletteColorInput.h"
#include "DragAndDropPayload.h"
#include "palette/Palette.h"
#include "ui/IMGUIEx.h"

namespace voxelui {

static int effectiveMaxIndex(const palette::Palette *palette, int maxIndex) {
	if (maxIndex >= 0) {
		return maxIndex;
	}
	if (palette != nullptr) {
		return palette->colorCount() - 1;
	}
	return 255;
}

static bool indexInRange(int val, const palette::Palette *palette, int minIndex, int maxIndex) {
	if (val < minIndex || val > effectiveMaxIndex(palette, maxIndex)) {
		return false;
	}
	if (palette != nullptr && val >= 0 && val >= palette->colorCount()) {
		return false;
	}
	return true;
}

bool paletteColorIndexInput(const char *label, int *index, const palette::Palette *palette, int minIndex,
							int maxIndex, bool readOnly) {
	if (index == nullptr) {
		return false;
	}

	int val = *index;
	if (palette != nullptr && val >= 0 && val < palette->colorCount()) {
		const float size = ImGui::Height(1);
		const ImVec2 v1 = ImGui::GetCursorScreenPos();
		const ImVec2 v2(v1.x + size, v1.y + size);
		ImDrawList *drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(v1, v2, ImGui::GetColorU32(palette->color(val)));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + size);
	}

	bool changed = false;
	const ImGuiInputTextFlags flags = readOnly ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;
	if (ImGui::InputIntWithButtons(label, &val, 1, 100, flags) && !readOnly) {
		if (indexInRange(val, palette, minIndex, maxIndex)) {
			*index = val;
			changed = true;
		}
	}
	if (!readOnly && palette != nullptr && ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(voxelui::dragdrop::PaletteIndexPayload)) {
			const int palIdx = *(const uint8_t *)payload->Data;
			if (indexInRange(palIdx, palette, minIndex, maxIndex)) {
				*index = palIdx;
				changed = true;
			}
		}
		ImGui::EndDragDropTarget();
	}
	return changed;
}

} // namespace voxelui
