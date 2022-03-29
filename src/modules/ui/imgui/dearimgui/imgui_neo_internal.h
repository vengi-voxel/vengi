//
// Created by Matty on 2022-01-28.
//

#ifndef IMGUI_NEO_INTERNAL_H
#define IMGUI_NEO_INTERNAL_H

#include "imgui.h"
#include "imgui_internal.h"
#include <cstdint>

namespace ImGui {
    IMGUI_API void  RenderNeoSequencerBackground(const ImVec4& color, const ImVec2 & cursor, const ImVec2& size, ImDrawList * drawList = nullptr, float sequencerRounding = 0.0f);
    IMGUI_API void  RenderNeoSequencerTopBarBackground(const ImVec4& color, const ImVec2 & cursor, const ImVec2& size, ImDrawList * drawList = nullptr, float sequencerRounding = 0.0f);
    IMGUI_API void  RenderNeoSequencerTopBarOverlay(float zoom, float valuesWidth,uint32_t startFrame, uint32_t endFrame, uint32_t offsetFrame, const ImVec2 &cursor, const ImVec2& size, ImDrawList * drawList = nullptr, bool drawFrameLines = true, bool drawFrameText = true);
    IMGUI_API void  RenderNeoTimelineLabel(const char * label,const ImVec2 & cursor,const ImVec2 & size, const ImVec4& color,bool isGroup = false, bool isOpen = false, ImDrawList *drawList = nullptr );
    IMGUI_API void  RenderNeoTimelane(bool selected,const ImVec2 & cursor, const ImVec2& size, const ImVec4& highlightColor, ImDrawList *drawList = nullptr);
    IMGUI_API void  RenderNeoTimelinesBorder(const ImVec4& color, const ImVec2 & cursor, const ImVec2& size, ImDrawList * drawList = nullptr, float rounding = 0.0f, float borderSize = 1.0f);
    IMGUI_API void  RenderNeoSequencerCurrentFrame(const ImVec4& color,const ImVec4 & topColor,const ImRect & pointerBB ,float timelineHeight, float lineWidth = 1.0f, ImDrawList * drawList = nullptr);

    IMGUI_API float GetPerFrameWidth(float totalSizeX, float valuesWidth, uint32_t endFrame, uint32_t startFrame, float zoom);
}

#endif //IMGUI_NEO_INTERNAL_H
