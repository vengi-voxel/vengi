//
// Created by Matty on 2022-01-28.
//

#ifndef IMGUI_NEO_SEQUENCER_H
#define IMGUI_NEO_SEQUENCER_H

#include "imgui.h"

typedef int ImGuiNeoSequencerFlags;
typedef int ImGuiNeoSequencerCol;
typedef int ImGuiNeoTimelineFlags;
typedef int ImGuiNeoTimelineIsSelectedFlags;

// Flags for ImGui::BeginNeoSequencer()
enum ImGuiNeoSequencerFlags_ {
	ImGuiNeoSequencerFlags_None = 0,
	ImGuiNeoSequencerFlags_AllowLengthChanging = 1 << 0, // Allows changing length of sequence
	ImGuiNeoSequencerFlags_EnableSelection = 1 << 1,	 // Enables selection of keyframes
	ImGuiNeoSequencerFlags_HideZoom = 1 << 2,			 // Disables zoom bar
	// ImGuiNeoSequencerFlags_PH                 = 1 << 3, // PLACEHOLDER
	ImGuiNeoSequencerFlags_AlwaysShowHeader = 1 << 4, // Enables overlay header, keeping it visible when scrolling

	// Selection options, only work with enable selection flag
	ImGuiNeoSequencerFlags_Selection_EnableDragging = 1 << 5,
	ImGuiNeoSequencerFlags_Selection_EnableDeletion = 1 << 6,

};

// Flags for ImGui::BeginNeoTimeline()
enum ImGuiNeoTimelineFlags_ {
	ImGuiNeoTimelineFlags_None = 0,
	ImGuiNeoTimelineFlags_AllowFrameChanging = 1 << 0,
	ImGuiNeoTimelineFlags_Group = 1 << 1,
};

// Flags for ImGui::IsNeoTimelineSelected()
enum ImGuiNeoTimelineIsSelectedFlags_ {
	ImGuiNeoTimelineIsSelectedFlags_None = 0,
	ImGuiNeoTimelineIsSelectedFlags_NewlySelected = 1 << 0,
};

enum ImGuiNeoSequencerCol_ {
	ImGuiNeoSequencerCol_Bg,
	ImGuiNeoSequencerCol_TopBarBg,
	ImGuiNeoSequencerCol_SelectedTimeline,
	ImGuiNeoSequencerCol_TimelineBorder,
	ImGuiNeoSequencerCol_TimelinesBg,
	ImGuiNeoSequencerCol_FramePointer,
	ImGuiNeoSequencerCol_FramePointerHovered,
	ImGuiNeoSequencerCol_FramePointerPressed,
	ImGuiNeoSequencerCol_Keyframe,
	ImGuiNeoSequencerCol_KeyframeHovered,
	ImGuiNeoSequencerCol_KeyframePressed,
	ImGuiNeoSequencerCol_KeyframeSelected,
	ImGuiNeoSequencerCol_FramePointerLine,

	ImGuiNeoSequencerCol_ZoomBarBg,
	ImGuiNeoSequencerCol_ZoomBarSlider,
	ImGuiNeoSequencerCol_ZoomBarSliderHovered,
	ImGuiNeoSequencerCol_ZoomBarSliderEnds,
	ImGuiNeoSequencerCol_ZoomBarSliderEndsHovered,

	ImGuiNeoSequencerCol_SelectionBorder,
	ImGuiNeoSequencerCol_Selection,

	ImGuiNeoSequencerCol_COUNT
};

struct ImGuiNeoSequencerStyle {
	float SequencerRounding = 2.5f;	  // Corner rounding around whole sequencer
	float TopBarHeight = 0.0f;		  // Value <= 0.0f = Height is calculated by FontSize + FramePadding.y * 2.0f
	bool TopBarShowFrameLines = true; // Show line for every frame in top bar
	bool TopBarShowFrameTexts = true; // Show frame number every 10th frame
	ImVec2 ItemSpacing = {4.0f, 0.5f};
	float DepthItemSpacing = 10.0f; // Amount of text offset per depth level in timeline values
	float TopBarSpacing = 3.0f;		// Space between top bar and timeline
	float TimelineBorderSize = 1.0f;
	float CurrentFramePointerSize = 7.0f; // Size of pointing arrow above current frame line
	float CurrentFrameLineWidth = 1.0f;	  // Width of line showing current frame over timeline
	float ZoomHeightScale = 1.0f;		  // Scale of Zoom bar, base height is font size
	float CollidedKeyframeOffset = 3.5f;  // Offset on which colliding keyframes are rendered

	float MaxSizePerTick =
		4.0f; // Maximum amount of pixels per tick on timeline (if less pixels is present, ticks are not rendered)

	ImVec4 Colors[ImGuiNeoSequencerCol_COUNT];

	ImGuiKey ModRemoveKey = ImGuiMod_Ctrl; // Key mod which when held removes selected keyframes from present selection
	ImGuiKey ModAddKey = ImGuiMod_Shift;   // Key mod which when held adds selected keyframes to present selection

	ImGuiNeoSequencerStyle();
};

namespace ImGui {
IMGUI_API const ImVec4 &GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol idx);
IMGUI_API ImGuiNeoSequencerStyle &GetNeoSequencerStyle();

IMGUI_API void PushNeoSequencerStyleColor(ImGuiNeoSequencerCol idx, ImU32 col);
IMGUI_API void PushNeoSequencerStyleColor(ImGuiNeoSequencerCol idx, const ImVec4 &col);
IMGUI_API void PopNeoSequencerStyleColor(int count = 1);

IMGUI_API bool BeginNeoSequencer(const char *id, uint32_t *frame, uint32_t *startFrame, uint32_t *endFrame,
								 const ImVec2 &size = ImVec2(0, 0),
								 ImGuiNeoSequencerFlags flags = ImGuiNeoSequencerFlags_None);
IMGUI_API void EndNeoSequencer(); // Call only when BeginNeoSequencer() returns true!!

IMGUI_API bool BeginNeoGroup(const char *label, bool *open = nullptr);
IMGUI_API void EndNeoGroup();

IMGUI_API bool BeginNeoTimeline(const char *label, int32_t **keyframes, uint32_t keyframeCount, bool *open = nullptr,
								ImGuiNeoTimelineFlags flags = ImGuiNeoTimelineFlags_None);
IMGUI_API void EndNeoTimeLine(); // Call only when BeginNeoTimeline() returns true!!

// Fully customizable timeline with per key callback
IMGUI_API bool BeginNeoTimelineEx(const char *label, bool *open = nullptr,
								  ImGuiNeoTimelineFlags flags = ImGuiNeoTimelineFlags_None);
IMGUI_API void NeoKeyframe(int32_t *value);

IMGUI_API bool IsNeoKeyframeHovered();
IMGUI_API bool IsNeoKeyframeSelected();
IMGUI_API bool IsNeoKeyframeRightClicked();

// Selection API
// DON'T delete keyframes while dragging, internal buffer will get corrupted
// Order for deletion is generally:
// CanDelete? -> DataSize? -> GetData() -> Delete your data -> ClearSelection()
IMGUI_API void NeoClearSelection();					 // Clears selection
IMGUI_API bool NeoIsSelecting();					 // Are we currently selecting?
IMGUI_API bool NeoHasSelection();					 // Is anything selected?
IMGUI_API bool NeoIsDraggingSelection();			 // Are we dragging selection?
IMGUI_API bool NeoCanDeleteSelection();				 // Can selection deletion be done?
IMGUI_API bool IsNeoKeyframeSelectionRightClicked(); // Is selection rightclicked?

// Call only in BeginNeoTimeline / EndNeoTimeLine scope, returns selection per timeline and size per timeline
IMGUI_API uint32_t GetNeoKeyframeSelectionSize();
IMGUI_API void GetNeoKeyframeSelection(int32_t *selection);

// Sets currently selected timeline inside BeginNeoSequencer scope
IMGUI_API void SetSelectedTimeline(const char *timelineLabel);

IMGUI_API bool IsNeoTimelineSelected(ImGuiNeoTimelineIsSelectedFlags flags = ImGuiNeoTimelineIsSelectedFlags_None);

IMGUI_API bool NeoBeginCreateKeyframe(int32_t *frame);

} // namespace ImGui

#endif // IMGUI_NEO_SEQUENCER_H
