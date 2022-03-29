//
// Created by Matty on 2022-01-28.
//

#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui_neo_internal.h"
#include "imgui_internal.h"
#include <cstdint>

namespace ImGui {
    void RenderNeoSequencerBackground(const ImVec4 &color, const ImVec2 & cursor, const ImVec2 &size, ImDrawList * drawList, float sequencerRounding) {
        if(!drawList) drawList = ImGui::GetWindowDrawList();

        const ImRect area = {cursor, cursor + size};

        drawList->AddRectFilled(area.Min, area.Max, ColorConvertFloat4ToU32(color), sequencerRounding);
    }

    void RenderNeoSequencerTopBarBackground(const ImVec4 &color, const ImVec2 &cursor, const ImVec2 &size,
                                            ImDrawList *drawList, float sequencerRounding) {
        if(!drawList) drawList = ImGui::GetWindowDrawList();

        const ImRect barArea = {cursor, cursor + size};

        drawList->AddRectFilled(barArea.Min, barArea.Max, ColorConvertFloat4ToU32(color), sequencerRounding);
    }

    void
    RenderNeoSequencerTopBarOverlay(float zoom, float valuesWidth,uint32_t startFrame, uint32_t endFrame, uint32_t offsetFrame, const ImVec2 &cursor, const ImVec2 &size,
                                    ImDrawList *drawList, bool drawFrameLines,
                                    bool drawFrameText) {
        if(!drawList) drawList = ImGui::GetWindowDrawList();

        const auto & style = GetStyle();

        const ImRect barArea = {cursor + ImVec2{style.FramePadding.x + valuesWidth,style.FramePadding.y}, cursor + size };

        const uint32_t viewEnd = endFrame + offsetFrame;
        const uint32_t viewStart = startFrame + offsetFrame;

        if(drawFrameLines) {
            const auto count = (int32_t)((float)((viewEnd + 1) - viewStart) / zoom);

            const auto perFrameWidth = barArea.GetSize().x / (float)count;

            for(int32_t i = 0; i < count; i++) {
                const ImVec2 p1 = {barArea.Min.x + (float)i * perFrameWidth, barArea.Max.y};

                const auto tenthFrame = ((viewStart + i) % 10 == 0);

                const auto lineHeight = tenthFrame ? barArea.GetSize().y : barArea.GetSize().y / 2.0f;

                const ImVec2 p2 = {barArea.Min.x + (float)i * perFrameWidth, barArea.Max.y - lineHeight};

                drawList->AddLine(p1,p2, IM_COL32_WHITE, 1.0f);

                if(drawFrameText && tenthFrame) {
                    char text[10];
                    const auto printRes = snprintf(text, sizeof(text), "%u", viewStart + i);
                    if(printRes > 0) {
                        drawList->AddText(NULL, 0, {p1.x + 2.0f, barArea.Min.y }, IM_COL32_WHITE,text);
                    }
                }
            }
        }
    }

    void RenderNeoTimelineLabel(const char * label,const ImVec2 & cursor,const ImVec2 & size, const ImVec4& color,bool isGroup, bool isOpen, ImDrawList *drawList)
    {
        const auto& imStyle = GetStyle();

        if(!drawList) drawList = ImGui::GetWindowDrawList();

        auto c = cursor;

        if(isGroup) {
            RenderArrow(drawList,c,IM_COL32_WHITE,isOpen ? ImGuiDir_Down : ImGuiDir_Right);
            c.x += size.y + imStyle.ItemSpacing.x;
        }

        drawList->AddText(c,ColorConvertFloat4ToU32(color),label);
    }

    void RenderNeoTimelinesBorder(const ImVec4 &color, const ImVec2 &cursor, const ImVec2 &size, ImDrawList *drawList,
                                  float rounding, float borderSize)
    {
        if(!drawList) drawList = ImGui::GetWindowDrawList();

        drawList->AddRect(cursor,cursor + size,ColorConvertFloat4ToU32(color),rounding, 0, borderSize);
    }

    void RenderNeoTimelane(bool selected,const ImVec2 & cursor, const ImVec2& size, const ImVec4& highlightColor, ImDrawList *drawList) {
        if(!drawList) drawList = ImGui::GetWindowDrawList();

        if(selected) {
            const ImRect area = {cursor, cursor + size};
            drawList->AddRectFilled(area.Min, area.Max, ColorConvertFloat4ToU32(highlightColor));
        }
    }

    float GetPerFrameWidth(float totalSizeX, float valuesWidth, uint32_t endFrame, uint32_t startFrame, float zoom) {
        const auto& imStyle = GetStyle();

        const auto size = totalSizeX - valuesWidth - imStyle.FramePadding.x;

        const auto count = (endFrame + 1) - startFrame;

        return (size / (float)count) * zoom;
    }

    struct Vec2Pair {
        ImVec2 a;
        ImVec2 b;
    };

    static Vec2Pair getCurrentFrameLine(const ImRect & pointerBB, float timelineHeight) {
        const auto center = ImVec2{pointerBB.Min.x, pointerBB.Max.y} + ImVec2{pointerBB.GetSize().x / 2.0f, 0};

        return Vec2Pair{ center, center + ImVec2{0, timelineHeight} };
    }

    void RenderNeoSequencerCurrentFrame(const ImVec4 &color, const ImVec4 &topColor, const ImRect &pointerBB,
                                               float timelineHeight, float lineWidth, ImDrawList *drawList) {
        if(!drawList) drawList = ImGui::GetWindowDrawList();

        const auto pair = getCurrentFrameLine(pointerBB, timelineHeight);

        drawList->AddLine(pair.a, pair.b, ColorConvertFloat4ToU32(color), lineWidth);

        { //Top pointer has custom shape, we have to create it
            const auto size = pointerBB.GetSize();
            ImVec2 pts[5];
            pts[0] = pointerBB.Min;
            pts[1] = pointerBB.Min + ImVec2{size.x, 0};
            pts[2] = pointerBB.Min + ImVec2{size.x, size.y * 0.85f};
            pts[3] = pointerBB.Min + ImVec2{size.x / 2, size.y};
            pts[4] = pointerBB.Min + ImVec2{0, size.y * 0.85f};

            drawList->AddConvexPolyFilled(pts, sizeof(pts) / sizeof(*pts), ColorConvertFloat4ToU32(topColor));
        }
    }
}
