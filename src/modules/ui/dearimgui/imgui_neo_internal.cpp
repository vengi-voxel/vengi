//
// Created by Matty on 2022-01-28.
//

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_neo_internal.h"
#include "imgui_internal.h"
#include <cstdint>

namespace ImGui {
void RenderNeoSequencerBackground(const ImVec4 &color, const ImVec2 &cursor, const ImVec2 &size, ImDrawList *drawList,
								  float sequencerRounding) {
	if (!drawList)
		drawList = ImGui::GetWindowDrawList();

	const ImRect area = {cursor, cursor + size};

	drawList->AddRectFilled(area.Min, area.Max, ColorConvertFloat4ToU32(color), sequencerRounding);
}

void RenderNeoSequencerTopBarBackground(const ImVec4 &color, const ImVec2 &cursor, const ImVec2 &size,
										ImDrawList *drawList, float sequencerRounding) {
	if (!drawList)
		drawList = ImGui::GetWindowDrawList();

	const ImRect barArea = {cursor, cursor + size};

	drawList->AddRectFilled(barArea.Min, barArea.Max, ColorConvertFloat4ToU32(color), sequencerRounding);
}

void RenderNeoSequencerTopBarOverlay(float zoom, float valuesWidth, uint32_t startFrame, uint32_t endFrame,
									 uint32_t offsetFrame, const ImVec2 &cursor, const ImVec2 &size,
									 ImDrawList *drawList, bool drawFrameLines, bool drawFrameText,
									 float maxPixelsPerTick) {
	if (!drawList)
		drawList = ImGui::GetWindowDrawList();

	const ImGuiStyle &style = GetStyle();

	const ImRect barArea = {cursor + ImVec2{style.FramePadding.x + valuesWidth, style.FramePadding.y}, cursor + size};

	const uint32_t viewStart = startFrame + offsetFrame;
	const uint32_t count = (uint32_t)((float)(endFrame - startFrame + 1) / zoom);
	const float perFrameWidth = GetPerFrameWidth(size.x, valuesWidth, endFrame, startFrame, zoom);
	if (!drawFrameLines || perFrameWidth <= 0.0f) {
		return;
	}

	// Visit visible ticks rather than every frame when the view is zoomed out.
	uint32_t step = 1;
	while (perFrameWidth * (float)step < maxPixelsPerTick && step < 100000000) {
		step *= 10;
	}
	const uint32_t first = (viewStart + step - 1) / step * step;
	drawList->PushClipRect(barArea.Min, barArea.Max, true);
	for (uint32_t frame = first; frame < viewStart + count; frame += step) {
		const bool major = frame % (step * 10) == 0;
		const float lineHeight = major ? barArea.GetHeight() : barArea.GetHeight() / 2.0f;
		const float x = barArea.Min.x + (float)(frame - viewStart) * perFrameWidth;
		drawList->AddLine(ImVec2(x, barArea.Max.y), ImVec2(x, barArea.Max.y - lineHeight), IM_COL32_WHITE);
		if (drawFrameText && major) {
			char text[16];
			snprintf(text, sizeof(text), "%u", frame);
			drawList->AddText(ImVec2(x + 2.0f, barArea.Min.y), IM_COL32_WHITE, text);
		}
	}
	drawList->PopClipRect();
}

void RenderNeoTimelineLabel(const char *label, const ImVec2 &cursor, const ImVec2 &size, const ImVec4 &color,
							bool isGroup, bool isOpen, ImDrawList *drawList) {
	const ImGuiStyle &imStyle = GetStyle();

	if (!drawList)
		drawList = ImGui::GetWindowDrawList();

	ImVec2 c = cursor;

	if (isGroup) {
		RenderArrow(drawList, c, IM_COL32_WHITE, isOpen ? ImGuiDir_Down : ImGuiDir_Right);
		c.x += size.y + imStyle.ItemSpacing.x;
	}

	drawList->AddText(c, ColorConvertFloat4ToU32(color), label, FindRenderedTextEnd(label));
}

void RenderNeoTimelinesBorder(const ImVec4 &color, const ImVec2 &cursor, const ImVec2 &size, ImDrawList *drawList,
							  float rounding, float borderSize) {
	if (!drawList)
		drawList = ImGui::GetWindowDrawList();

	drawList->AddRect(cursor, cursor + size, ColorConvertFloat4ToU32(color), rounding, borderSize);
}

void RenderNeoTimelane(bool selected, const ImVec2 &cursor, const ImVec2 &size, const ImVec4 &highlightColor,
					   ImDrawList *drawList) {
	if (!drawList)
		drawList = ImGui::GetWindowDrawList();

	if (selected) {
		const ImRect area = {cursor, cursor + size};
		drawList->AddRectFilled(area.Min, area.Max, ColorConvertFloat4ToU32(highlightColor));
	}
}

float GetPerFrameWidth(float totalSizeX, float valuesWidth, uint32_t endFrame, uint32_t startFrame, float zoom) {
	const ImGuiStyle &imStyle = GetStyle();

	const float size = totalSizeX - valuesWidth - imStyle.FramePadding.x;

	uint32_t count = (endFrame + 1) - startFrame;

	return ((size / (float)count) * zoom);
}

struct Vec2Pair {
	ImVec2 a;
	ImVec2 b;
};

static Vec2Pair getCurrentFrameLine(const ImRect &pointerBB, float timelineHeight) {
	const ImVec2 center = ImVec2{pointerBB.Min.x, pointerBB.Max.y} + ImVec2{pointerBB.GetSize().x / 2.0f, 0};

	return Vec2Pair{center, center + ImVec2{0, timelineHeight}};
}

void RenderNeoSequencerCurrentFrame(const ImVec4 &color, const ImVec4 &topColor, const ImRect &pointerBB,
									float timelineHeight, float lineWidth, ImDrawList *drawList) {
	if (!drawList)
		drawList = ImGui::GetWindowDrawList();

	const Vec2Pair pair = getCurrentFrameLine(pointerBB, timelineHeight);

	drawList->AddLine(pair.a, pair.b, ColorConvertFloat4ToU32(color), lineWidth);

	{ // Top pointer has custom shape, we have to create it
		const ImVec2 size = pointerBB.GetSize();
		ImVec2 pts[5];
		pts[0] = pointerBB.Min;
		pts[1] = pointerBB.Min + ImVec2{size.x, 0};
		pts[2] = pointerBB.Min + ImVec2{size.x, size.y * 0.85f};
		pts[3] = pointerBB.Min + ImVec2{size.x / 2, size.y};
		pts[4] = pointerBB.Min + ImVec2{0, size.y * 0.85f};

		drawList->AddConvexPolyFilled(pts, sizeof(pts) / sizeof(*pts), ColorConvertFloat4ToU32(topColor));
	}
}
} // namespace ImGui
