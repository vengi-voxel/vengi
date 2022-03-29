//
// Created by Matty on 2022-01-28.
//

#ifndef IMGUI_NEO_SEQUENCER_H
#define IMGUI_NEO_SEQUENCER_H

#include "imgui.h"
#include <vector>

typedef int ImGuiNeoSequencerFlags;
typedef int ImGuiNeoSequencerCol;
typedef int ImGuiNeoTimelineFlags;

// Flags for ImGui::BeginNeoSequencer()
enum ImGuiNeoSequencerFlags_
{
    ImGuiNeoSequencerFlags_None                 = 0     ,
    ImGuiNeoSequencerFlags_AllowLengthChanging  = 1 << 0,
    ImGuiNeoSequencerFlags_AllowSelection       = 1 << 1,
    ImGuiNeoSequencerFlags_HideZoom             = 1 << 2,
    ImGuiNeoSequencerFlags_ZoomBottomOverlay    = 1 << 3,

};

// Flags for ImGui::BeginNeoTimeline()
enum ImGuiNeoTimelineFlags_
{
    ImGuiNeoTimelineFlags_None                 = 0     ,
    ImGuiNeoTimelineFlags_AllowFrameChanging   = 1 << 0,
    ImGuiNeoTimelineFlags_Group                = 1 << 1,
};

enum ImGuiNeoSequencerCol_
{
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
    ImGuiNeoSequencerCol_FramePointerLine,

    ImGuiNeoSequencerCol_ZoomBarBg,
    ImGuiNeoSequencerCol_ZoomBarSlider,
    ImGuiNeoSequencerCol_ZoomBarSliderHovered,
    ImGuiNeoSequencerCol_ZoomBarSliderEnds,
    ImGuiNeoSequencerCol_ZoomBarSliderEndsHovered,
    ImGuiNeoSequencerCol_COUNT
};

struct ImGuiNeoSequencerStyle {
    float       SequencerRounding = 2.5f;       // Corner rounding around whole sequencer
    float       TopBarHeight = 0.0f;            // Value <= 0.0f = Height is calculated by FontSize + FramePadding.y * 2.0f
    bool        TopBarShowFrameLines = true;   // Show line for every frame in top bar
    bool        TopBarShowFrameTexts = true;    // Show frame number every 10th frame
    ImVec2      ItemSpacing = {4.0f,0.5f};
    float       DepthItemSpacing = 10.0f;       // Amount of text offset per depth level in timeline values
    float       TopBarSpacing = 3.0f;           // Space between top bar and timeline
    float       TimelineBorderSize = 1.0f;
    float       CurrentFramePointerSize = 7.0f; // Size of pointing arrow above current frame line
    float       CurrentFrameLineWidth = 1.0f; // Width of line showing current frame over timeline
    float       ZoomHeightScale = 1.0f;         // Scale of Zoom bar, base height is font size

    ImVec4      Colors[ImGuiNeoSequencerCol_COUNT];

    ImGuiNeoSequencerStyle();
};

namespace ImGui {
    IMGUI_API const ImVec4& GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol idx);
    IMGUI_API ImGuiNeoSequencerStyle& GetNeoSequencerStyle();

    IMGUI_API void PushNeoSequencerStyleColor(ImGuiNeoSequencerCol idx, ImU32 col);
    IMGUI_API void PushNeoSequencerStyleColor(ImGuiNeoSequencerCol idx, const ImVec4& col);
    IMGUI_API void PopNeoSequencerStyleColor(int count = 1);

    IMGUI_API bool BeginNeoSequencer(const char* id, uint32_t * frame, uint32_t * startFrame, uint32_t * endFrame,const ImVec2& size = ImVec2(0, 0),ImGuiNeoSequencerFlags flags = ImGuiNeoSequencerFlags_None);
    IMGUI_API void EndNeoSequencer(); //Call only when BeginNeoSequencer() returns true!!

    IMGUI_API bool BeginNeoGroup(const char* label, bool* open = nullptr);
    IMGUI_API void EndNeoGroup();

    IMGUI_API bool BeginNeoTimeline(const char* label,uint32_t ** keyframes, uint32_t keyframeCount, bool * open = nullptr, ImGuiNeoTimelineFlags flags = ImGuiNeoTimelineFlags_None);
    IMGUI_API void EndNeoTimeLine(); //Call only when BeginNeoTimeline() returns true!!

    IMGUI_API bool NeoBeginCreateKeyframe(uint32_t * frame);

#ifdef __cplusplus
    // C++ helper
    IMGUI_API bool BeginNeoTimeline(const char* label,std::vector<uint32_t> & keyframes ,bool * open = nullptr);
#endif
}


#endif //IMGUI_NEO_SEQUENCER_H
