//
// Created by Matty on 2022-01-28.
//
#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui_neo_sequencer.h"
#include "imgui_internal.h"
#include "imgui_neo_internal.h"

#include <unordered_map>

namespace ImGui {
struct ImGuiNeoSequencerInternalData {
	ImVec2 TopLeftCursor = {0, 0};	   // Cursor on top of whole widget
	ImVec2 TopBarStartCursor = {0, 0}; // Cursor on top, below Zoom slider
	ImVec2 StartValuesCursor = {0, 0}; // Cursor on top of values
	ImVec2 ValuesCursor = {0, 0};	   // Current cursor position, used for values drawing

	ImVec2 Size = {0, 0};		// Size of whole sequencer
	ImVec2 TopBarSize = {0, 0}; // Size of top bar without Zoom

	uint32_t StartFrame = 0;
	uint32_t EndFrame = 0;
	uint32_t OffsetFrame = 0; // Offset from start

	float ValuesWidth = 32.0f; // Width of biggest label in timeline, used for offset of timeline

	float FilledHeight = 0.0f; // Height of whole sequencer

	float Zoom = 1.0f;

	ImGuiID LastSelectedTimeline = 0;
	ImGuiID SelectedTimeline = 0;

	ImVector<ImGuiID> TimelineStack;

	uint32_t CurrentFrame = 0;
	bool HoldingCurrentFrame = false; // Are we draging current frame?
	ImVec4 CurrentFrameColor; // Color of current frame, we have to save it because we render on EndNeoSequencer, but
							  // process at BeginneoSequencer

	bool HoldingZoomSlider = false;
};

static ImGuiNeoSequencerStyle style; // NOLINT(cert-err58-cpp)

// Global context stuff
static bool inSequencer = false;

// Height of timeline right now
static float currentTimelineHeight = 0.0f;

// Current active sequencer
static ImGuiID currentSequencer;

// Current timeline depth, used for offset of label
static uint32_t currentTimelineDepth = 0;

static ImVector<ImGuiColorMod> sequencerColorStack;

// Data of all sequencers, this is main c++ part and I should create C alternative or use imgui ImVector or something
static std::unordered_map<ImGuiID, ImGuiNeoSequencerInternalData> sequencerData;

///////////// STATIC HELPERS ///////////////////////

static float getPerFrameWidth(ImGuiNeoSequencerInternalData &context) {
	return GetPerFrameWidth(context.Size.x, context.ValuesWidth, context.EndFrame, context.StartFrame, context.Zoom);
}

static float getKeyframePositionX(uint32_t frame, ImGuiNeoSequencerInternalData &context) {
	const auto perFrameWidth = getPerFrameWidth(context);
	return (float)(frame - context.OffsetFrame) * perFrameWidth;
}

static float getWorkTimelineWidth(ImGuiNeoSequencerInternalData &context) {
	const auto perFrameWidth = getPerFrameWidth(context);
	return context.Size.x - context.ValuesWidth - perFrameWidth;
}

// Dont pull frame from context, its used for dragging
static ImRect getCurrentFrameBB(uint32_t frame, ImGuiNeoSequencerInternalData &context) {
	const auto &imStyle = GetStyle();
	const auto width = style.CurrentFramePointerSize * GetIO().FontGlobalScale;
	const auto cursor =
		context.TopBarStartCursor + ImVec2{context.ValuesWidth + imStyle.FramePadding.x - width / 2.0f, 0};
	const auto currentFrameCursor = cursor + ImVec2{getKeyframePositionX(frame, context), 0};

	float pointerHeight = style.CurrentFramePointerSize * 2.5f;
	ImRect rect{currentFrameCursor, currentFrameCursor + ImVec2{width, pointerHeight * GetIO().FontGlobalScale}};

	return rect;
}

static void processCurrentFrame(uint32_t *frame, ImGuiNeoSequencerInternalData &context) {
	auto pointerRect = getCurrentFrameBB(*frame, context);
	pointerRect.Min -= ImVec2{2.0f, 2.0f};
	pointerRect.Max += ImVec2{2.0f, 2.0f};

	const auto &imStyle = GetStyle();

	const auto timelineXmin = context.TopBarStartCursor.x + context.ValuesWidth + imStyle.FramePadding.x;

	const ImVec2 timelineXRange = {timelineXmin, // min
								   timelineXmin + context.Size.x - context.ValuesWidth};

	const auto hovered = ItemHoverable(pointerRect, GetCurrentWindow()->GetID("##_top_selector_neo"));

	context.CurrentFrameColor = GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_FramePointer);

	if (hovered) {
		context.CurrentFrameColor = GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_FramePointerHovered);
	}

	if (context.HoldingCurrentFrame) {
		if (IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
			const auto mousePosX = GetMousePos().x;
			const auto v = mousePosX - timelineXRange.x; // Subtract min

			const auto normalized = v / getWorkTimelineWidth(context); // Divide by width to remap to 0 - 1 range

			const auto clamped = ImClamp(normalized, 0.0f, 1.0f);

			const auto viewSize = (float)(context.EndFrame - context.StartFrame) / context.Zoom;

			const auto frameViewVal = (float)context.StartFrame + (clamped * (float)viewSize);

			const auto finalFrame = (uint32_t)round(frameViewVal) + context.OffsetFrame;

			context.CurrentFrameColor = GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_FramePointerPressed);

			*frame = finalFrame;
		}

		if (!IsMouseDown(ImGuiMouseButton_Left)) {
			context.HoldingCurrentFrame = false;
			context.CurrentFrameColor = GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_FramePointer);
		}
	}

	if (hovered && IsMouseDown(ImGuiMouseButton_Left) && !context.HoldingCurrentFrame) {
		context.HoldingCurrentFrame = true;
		context.CurrentFrameColor = GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_FramePointerPressed);
	}

	context.CurrentFrame = *frame;
}

static void finishPreviousTimeline(ImGuiNeoSequencerInternalData &context) {
	context.ValuesCursor = {context.TopBarStartCursor.x, context.ValuesCursor.y};
	currentTimelineHeight = 0.0f;
}

static bool createKeyframe(uint32_t *frame) {
	const auto &imStyle = GetStyle();
	auto &context = sequencerData[currentSequencer];

	const auto timelineOffset = getKeyframePositionX(*frame, context);

	const auto pos = ImVec2{context.StartValuesCursor.x + imStyle.FramePadding.x, context.ValuesCursor.y} +
					 ImVec2{timelineOffset + context.ValuesWidth, 0};

	const auto bbPos = pos - ImVec2{currentTimelineHeight / 2, 0};

	const ImRect bb = {bbPos, bbPos + ImVec2{currentTimelineHeight, currentTimelineHeight}};

	const auto drawList = ImGui::GetWindowDrawList();

	bool hovered = ItemHoverable(bb, GetCurrentWindow()->GetID(frame));

	drawList->AddCircleFilled(
		pos + ImVec2{0, currentTimelineHeight / 2.f}, currentTimelineHeight / 3.0f,
		hovered ? ColorConvertFloat4ToU32(GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_KeyframeHovered))
				: ColorConvertFloat4ToU32(GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_Keyframe)),
		4);

	return true;
}

static uint32_t idCounter = 0;
static char idBuffer[16];

const char *generateID() {
	idBuffer[0] = '#';
	idBuffer[1] = '#';
	memset(idBuffer + 2, 0, 14);
	snprintf(idBuffer + 2, 14, "%o", idCounter++);

	return &idBuffer[0];
}

void resetID() {
	idCounter = 0;
}

static void renderCurrentFrame(ImGuiNeoSequencerInternalData &context) {
	const auto bb = getCurrentFrameBB(context.CurrentFrame, context);

	const auto drawList = ImGui::GetWindowDrawList();

	RenderNeoSequencerCurrentFrame(GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_FramePointerLine),
								   context.CurrentFrameColor, bb, context.Size.y - context.TopBarSize.y,
								   style.CurrentFrameLineWidth, drawList);
}

static float calculateZoomBarHeight() {
	const auto &imStyle = GetStyle();
	return GetFontSize() * style.ZoomHeightScale + imStyle.FramePadding.y * 2.0f;
}

static void processAndRenderZoom(ImGuiNeoSequencerInternalData &context, const ImVec2 &cursor, bool allowEditingLength,
								 uint32_t *start, uint32_t *end) {
	const auto &imStyle = GetStyle();
	ImGuiWindow *window = GetCurrentWindow();

	const auto zoomHeight = calculateZoomBarHeight();

	auto *drawList = GetWindowDrawList();

	// Input width
	const auto inputWidth = CalcTextSize("123456").x;

	const auto inputWidthWithPadding = inputWidth + imStyle.ItemSpacing.x;

	const auto cursorV = allowEditingLength ? cursor + ImVec2{inputWidthWithPadding, 0} : cursor;

	const auto size = allowEditingLength ? context.Size.x - 2 * inputWidthWithPadding : context.Size.x;

	const ImRect bb{cursorV, cursorV + ImVec2{size, zoomHeight}};

	const auto zoomBarEndWithSpacing = ImVec2{bb.Max.x + imStyle.ItemSpacing.x, bb.Min.y};

	int32_t startFrameVal = (int32_t)*start;
	int32_t endFrameVal = (int32_t)*end;

	if (allowEditingLength) {
		const float sideOffset = imStyle.ItemSpacing.x / 2.0f;
		auto prevWindowCursor = window->DC.CursorPos;

		window->DC.CursorPos = cursor;

		window->DC.CursorPos.x += sideOffset;

		PushItemWidth(inputWidth);
		InputScalar("##input_start_frame", ImGuiDataType_U32, &startFrameVal, NULL, NULL, NULL,
					allowEditingLength ? 0 : ImGuiInputTextFlags_ReadOnly);

		window->DC.CursorPos = ImVec2{zoomBarEndWithSpacing.x, cursor.y};
		window->DC.CursorPos.x -= sideOffset;

		PushItemWidth(inputWidth);
		InputScalar("##input_end_frame", ImGuiDataType_U32, &endFrameVal, NULL, NULL, NULL,
					allowEditingLength ? 0 : ImGuiInputTextFlags_ReadOnly);

		window->DC.CursorPos = prevWindowCursor;
	}

	if (startFrameVal < 0)
		startFrameVal = (int32_t)*start;

	if (endFrameVal < 0)
		endFrameVal = (int32_t)*end;

	if (endFrameVal <= startFrameVal)
		endFrameVal = (int32_t)*end;

	*start = startFrameVal;
	*end = endFrameVal;

	// drawList->AddText(startFrameTextCursor + ImVec2{frameNumberBorderSize.x, 0} -
	// ImVec2{numberTextWidth,0},IM_COL32_WHITE,numberText);

	// Background
	drawList->AddRectFilled(
		bb.Min, bb.Max, ColorConvertFloat4ToU32(GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_ZoomBarBg)), 10.0f);

	const auto baseWidth =
		bb.GetSize().x -
		imStyle.ItemInnerSpacing.x; // There is just half spacing applied, doing it normally makes big gap on sides

	const auto sliderHeight = bb.GetSize().y - imStyle.ItemInnerSpacing.y;

	const auto sliderWidth = baseWidth / context.Zoom;

	const auto sliderMin = bb.Min + imStyle.ItemInnerSpacing / 2.0f;

	// const auto sliderMax = bb.Max - imStyle.ItemInnerSpacing / 2.0f;

	const auto sliderMaxWidth = baseWidth;

	const auto totalFrames = (*end - *start);

	const auto singleFrameWidthOffset = sliderMaxWidth / (float)totalFrames;

	const auto zoomSliderOffset = singleFrameWidthOffset * (float)context.OffsetFrame;

	const auto sliderStart = sliderMin + ImVec2{zoomSliderOffset, 0};

	const float sideSize = sliderHeight;

	const ImRect finalSliderBB{sliderStart, sliderStart + ImVec2{sliderWidth, sliderHeight}};

	const ImRect finalSliderInteractBB = {finalSliderBB.Min + ImVec2{sideSize, 0},
										  finalSliderBB.Max - ImVec2{sideSize, 0}};

	const auto viewWidth = (uint32_t)((float)totalFrames / context.Zoom);

	const bool hovered = ItemHoverable(bb, GetCurrentWindow()->GetID("##zoom_slider"));

	if (hovered) {
		SetKeyOwner(ImGuiKey_MouseWheelY, GetItemID());
		const float currentScroll = GetIO().MouseWheel;

		context.Zoom = ImClamp(context.Zoom + float(currentScroll) * 0.3f, 1.0f, (float)viewWidth);
		const auto newZoomWidth = (uint32_t)ceil((float)totalFrames / (context.Zoom));

		if (*start + context.OffsetFrame + newZoomWidth > *end)
			context.OffsetFrame = ImMax(0U, totalFrames - viewWidth);
	}

	if (context.HoldingZoomSlider) {
		if (IsMouseDragging(ImGuiMouseButton_Left, 0.01f)) {
			const auto currentX = GetMousePos().x;

			const auto v = currentX - bb.Min.x; // Subtract min

			const auto normalized = v / bb.GetWidth(); // Divide by width to remap to 0 - 1 range

			const auto sliderWidthNormalized = 1.0f / context.Zoom;

			const auto singleFrameWidthOffsetNormalized = singleFrameWidthOffset / bb.GetWidth();

			uint32_t finalFrame =
				(uint32_t)((float)(normalized - sliderWidthNormalized / 2.0f) / singleFrameWidthOffsetNormalized);

			if (normalized - sliderWidthNormalized / 2.0f < 0.0f) {
				finalFrame = 0;
			}

			if (normalized + sliderWidthNormalized / 2.0f > 1.0f) {
				finalFrame = totalFrames - viewWidth;
			}

			context.OffsetFrame = finalFrame;
		}

		if (!IsMouseDown(ImGuiMouseButton_Left)) {
			context.HoldingZoomSlider = false;
		}
	}

	if (hovered && IsMouseDown(ImGuiMouseButton_Left)) {
		context.HoldingZoomSlider = true;
	}

	const auto res = ItemAdd(finalSliderInteractBB, 0);

	const auto viewStart = *start + (uint32_t)context.OffsetFrame;
	const auto viewEnd = viewStart + viewWidth;

	if (res) {
		auto sliderColor = GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_ZoomBarSlider);

		if (IsItemHovered()) {
			sliderColor = GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_ZoomBarSliderHovered);
		}

		// Render bar
		drawList->AddRectFilled(finalSliderBB.Min, finalSliderBB.Max, ColorConvertFloat4ToU32(sliderColor), 10.0f);

		const auto sliderCenter = finalSliderBB.GetCenter();

		char overlayTextBuffer[128];

		snprintf(overlayTextBuffer, sizeof(overlayTextBuffer), "%d - %d", viewStart, viewEnd);

		const auto overlaySize = CalcTextSize(overlayTextBuffer);

		drawList->AddText(sliderCenter - overlaySize / 2.0f, IM_COL32_WHITE, overlayTextBuffer);
	}
}
////////////////////////////////////

const ImVec4 &GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol idx) {
	return GetNeoSequencerStyle().Colors[idx];
}

ImGuiNeoSequencerStyle &GetNeoSequencerStyle() {
	return style;
}

bool BeginNeoSequencer(const char *idin, uint32_t *frame, uint32_t *startFrame, uint32_t *endFrame, const ImVec2 &size,
					   ImGuiNeoSequencerFlags flags) {
	IM_ASSERT(!inSequencer && "Called when while in other NeoSequencer, that won't work, call End!");
	IM_ASSERT(*startFrame < *endFrame && "Start frame must be smaller than end frame");

	// ImGuiContext &g = *GImGui;
	ImGuiWindow *window = GetCurrentWindow();
	const auto &imStyle = GetStyle();
	// auto &neoStyle = GetNeoSequencerStyle();

	if (inSequencer)
		return false;

	if (window->SkipItems)
		return false;

	const auto drawList = GetWindowDrawList();

	const auto cursor = GetCursorScreenPos();
	const auto area = ImGui::GetContentRegionAvail();

	const auto cursorBasePos = GetCursorScreenPos() + window->Scroll;

	PushID(idin);
	const auto id = window->IDStack[window->IDStack.size() - 1];

	inSequencer = true;

	auto &context = sequencerData[id];

	auto realSize = ImFloor(size);
	if (realSize.x <= 0.0f)
		realSize.x = ImMax(4.0f, area.x);
	if (realSize.y <= 0.0f)
		realSize.y = ImMax(4.0f, context.FilledHeight);

	const bool showZoom = !(flags & ImGuiNeoSequencerFlags_HideZoom);
	const bool headerAlwaysVisible = (flags & ImGuiNeoSequencerFlags_AlwaysShowHeader);

	context.TopLeftCursor = headerAlwaysVisible ? cursorBasePos : cursor;

	// If Zoom is shown, we offset it by height of Zoom bar + padding
	context.TopBarStartCursor =
		showZoom ? context.TopLeftCursor + ImVec2{0, calculateZoomBarHeight()} : context.TopLeftCursor;
	context.StartFrame = *startFrame;
	context.EndFrame = *endFrame;
	context.Size = realSize;

	context.TopBarSize = ImVec2(context.Size.x, style.TopBarHeight);

	if (context.TopBarSize.y <= 0.0f)
		context.TopBarSize.y = CalcTextSize("100").y + imStyle.FramePadding.y * 2.0f;

	currentSequencer = window->IDStack[window->IDStack.size() - 1];

	auto backgroundSize = context.Size;
	const float topCut = abs(context.TopLeftCursor.y - cursor.y);
	backgroundSize.y = backgroundSize.y - (topCut);

	RenderNeoSequencerBackground(GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_Bg), context.TopLeftCursor,
								 backgroundSize, drawList, style.SequencerRounding);

	RenderNeoSequencerTopBarBackground(GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_TopBarBg),
									   context.TopBarStartCursor, context.TopBarSize, drawList,
									   style.SequencerRounding);

	RenderNeoSequencerTopBarOverlay(context.Zoom, context.ValuesWidth, context.StartFrame, context.EndFrame,
									context.OffsetFrame, context.TopBarStartCursor, context.TopBarSize, drawList,
									style.TopBarShowFrameLines, style.TopBarShowFrameTexts);

	if (showZoom)
		processAndRenderZoom(context, context.TopLeftCursor, flags & ImGuiNeoSequencerFlags_AllowLengthChanging,
							 startFrame, endFrame);

	if (context.Size.y < context.FilledHeight)
		context.Size.y = context.FilledHeight;

	context.FilledHeight = context.TopBarSize.y + style.TopBarSpacing + (showZoom ? calculateZoomBarHeight() : 0.0f);

	context.StartValuesCursor = cursor + ImVec2{0, context.TopBarSize.y + style.TopBarSpacing};
	if (showZoom)
		context.StartValuesCursor = context.StartValuesCursor + ImVec2{0, calculateZoomBarHeight()};
	context.ValuesCursor = context.StartValuesCursor;

	processCurrentFrame(frame, context);

	const auto clipMin = context.TopBarStartCursor + ImVec2(0, context.TopBarSize.y);

	drawList->PushClipRect(clipMin,
						   clipMin + backgroundSize - ImVec2(0, context.TopBarSize.y) -
							   ImVec2{0, GetFontSize() * style.ZoomHeightScale},
						   true);

	return true;
}

void EndNeoSequencer() {
	IM_ASSERT(inSequencer && "Called end sequencer when BeginSequencer didnt return true or wasn't called at all!");
	IM_ASSERT(sequencerData.count(currentSequencer) != 0 && "Ended sequencer has no context!");

	auto &context = sequencerData[currentSequencer];
	IM_ASSERT(context.TimelineStack.empty() && "Missmatch in timeline Begin / End");

	context.LastSelectedTimeline = context.SelectedTimeline;

	renderCurrentFrame(context);

	inSequencer = false;

	const ImVec2 min = {0, 0};
	context.Size.y = context.FilledHeight;
	const auto max = context.Size;

	ItemSize({min, max});
	PopID();
	resetID();
}

IMGUI_API bool BeginNeoGroup(const char *label, bool *open) {
	return BeginNeoTimeline(label, nullptr, 0, open, ImGuiNeoTimelineFlags_Group);
}

IMGUI_API void EndNeoGroup() {
	return EndNeoTimeLine();
}

static bool groupBehaviour(const ImGuiID id, bool *open, const ImVec2 labelSize) {
	auto &context = sequencerData[currentSequencer];
	ImGuiWindow *window = GetCurrentWindow();

	const bool closable = open != nullptr;

	auto drawList = ImGui::GetWindowDrawList();
	const float arrowWidth = drawList->_Data->FontSize;
	const ImVec2 arrowSize = {arrowWidth, arrowWidth};
	const ImRect arrowBB = {context.ValuesCursor, context.ValuesCursor + arrowSize};
	const ImVec2 groupBBMin = {context.ValuesCursor + ImVec2{arrowSize.x, 0.0f}};
	const ImRect groupBB = {groupBBMin, groupBBMin + labelSize};
	const ImGuiID arrowID = window->GetID(generateID());
	const auto addArrowRes = ItemAdd(arrowBB, arrowID);
	if (addArrowRes) {
		if (IsItemClicked() && closable)
			(*open) = !(*open);
	}

	const auto addGroupRes = ItemAdd(groupBB, id);
	if (addGroupRes) {
		if (IsItemClicked()) {
			context.LastSelectedTimeline = context.SelectedTimeline;
			context.SelectedTimeline = context.SelectedTimeline == id ? 0 : id;
		}
	}
	const float width = groupBB.Max.x - arrowBB.Min.x;
	context.ValuesWidth = std::max(context.ValuesWidth, width); // Make left panel wide enough
	return addGroupRes && addArrowRes;
}

static bool timelineBehaviour(const ImGuiID id, const ImVec2 labelSize) {
	auto &context = sequencerData[currentSequencer];
	// ImGuiWindow *window = GetCurrentWindow();

	const ImRect groupBB = {context.ValuesCursor, context.ValuesCursor + labelSize};

	const auto addGroupRes = ItemAdd(groupBB, id);
	if (addGroupRes) {
		if (IsItemClicked()) {
			context.LastSelectedTimeline = context.SelectedTimeline;
			context.SelectedTimeline = context.SelectedTimeline == id ? 0 : id;
		}
	}
	const float width = groupBB.Max.x - groupBB.Min.x;
	context.ValuesWidth = std::max(context.ValuesWidth, width); // Make left panel wide enough

	return addGroupRes;
}

bool BeginNeoTimeline(const char *label, uint32_t **keyframes, uint32_t keyframeCount, bool *open,
					  ImGuiNeoTimelineFlags flags) {
	IM_ASSERT(inSequencer && "Not in active sequencer!");

	const bool closable = open != nullptr;

	auto &context = sequencerData[currentSequencer];
	const auto &imStyle = GetStyle();
	ImGuiWindow *window = GetCurrentWindow();
	const ImGuiID id = window->GetID(label);
	auto labelSize = CalcTextSize(label);

	labelSize.y += imStyle.FramePadding.y * 2 + style.ItemSpacing.y * 2;
	labelSize.x +=
		imStyle.FramePadding.x * 2 + style.ItemSpacing.x * 2 + (float)currentTimelineDepth * style.DepthItemSpacing;

	bool isGroup = flags & ImGuiNeoTimelineFlags_Group && closable;
	bool addRes = false;
	if (isGroup) {
		labelSize.x += imStyle.ItemSpacing.x + GetFontSize();
		addRes = groupBehaviour(id, open, labelSize);
	} else {
		addRes = timelineBehaviour(id, labelSize);
	}

	if (currentTimelineDepth > 0) {
		context.ValuesCursor = {context.TopBarStartCursor.x, context.ValuesCursor.y};
	}

	currentTimelineHeight = labelSize.y;
	context.FilledHeight += currentTimelineHeight;

	if (addRes) {
		RenderNeoTimelane(id == context.SelectedTimeline, context.ValuesCursor + ImVec2{context.ValuesWidth, 0},
						  ImVec2{context.Size.x - context.ValuesWidth, currentTimelineHeight},
						  GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_SelectedTimeline));

		ImVec4 color = GetStyleColorVec4(ImGuiCol_Text);
		if (IsItemHovered())
			color.w *= 0.7f;

		RenderNeoTimelineLabel(label,
							   context.ValuesCursor + imStyle.FramePadding +
								   ImVec2{(float)currentTimelineDepth * style.DepthItemSpacing, 0},
							   labelSize, color, isGroup, isGroup && (*open));
	}

	for (uint32_t i = 0; i < keyframeCount; i++) {
		/*bool keyframeRes = */ createKeyframe(keyframes[i]);
	}

	context.ValuesCursor.x += imStyle.FramePadding.x + (float)currentTimelineDepth * style.DepthItemSpacing;
	context.ValuesCursor.y += currentTimelineHeight;

	const auto result = !closable || (*open);

	if (result) {
		currentTimelineDepth++;
		context.TimelineStack.push_back(id);
	} else {
		finishPreviousTimeline(context);
	}
	return result;
}

void EndNeoTimeLine() {
	auto &context = sequencerData[currentSequencer];
	finishPreviousTimeline(context);
	currentTimelineDepth--;
	context.TimelineStack.pop_back();
}

bool NeoBeginCreateKeyframe(uint32_t *frame) {
	return false;
}

void PushNeoSequencerStyleColor(ImGuiNeoSequencerCol idx, ImU32 col) {
	ImGuiColorMod backup;
	backup.Col = idx;
	backup.BackupValue = style.Colors[idx];
	sequencerColorStack.push_back(backup);
	style.Colors[idx] = ColorConvertU32ToFloat4(col);
}

void PushNeoSequencerStyleColor(ImGuiNeoSequencerCol idx, const ImVec4 &col) {
	ImGuiColorMod backup;
	backup.Col = idx;
	backup.BackupValue = style.Colors[idx];
	sequencerColorStack.push_back(backup);
	style.Colors[idx] = col;
}

void PopNeoSequencerStyleColor(int count) {
	while (count > 0) {
		ImGuiColorMod &backup = sequencerColorStack.back();
		style.Colors[backup.Col] = backup.BackupValue;
		sequencerColorStack.pop_back();
		count--;
	}
}

void SetSelectedTimeline(const char *timelineLabel) {
	IM_ASSERT(inSequencer && "Not in active sequencer!");

	auto &context = sequencerData[currentSequencer];

	ImGuiWindow *window = GetCurrentWindow();

	ImGuiID timelineID = 0;

	if (timelineLabel) {
		timelineID = window->GetID(timelineLabel);
	}
	context.LastSelectedTimeline = context.SelectedTimeline;
	context.SelectedTimeline = timelineID;
}

bool IsNeoTimelineSelected(ImGuiNeoTimelineIsSelectedFlags flags) {
	IM_ASSERT(inSequencer && "Not in active sequencer!");
	auto &context = sequencerData[currentSequencer];

	IM_ASSERT(!context.TimelineStack.empty() && "No active timelines are present!");

	const bool newly = flags & ImGuiNeoTimelineIsSelectedFlags_NewlySelected;

	const auto openTimeline = context.TimelineStack[context.TimelineStack.size() - 1];

	if (!newly) {
		return context.SelectedTimeline == openTimeline;
	}

	return (context.SelectedTimeline != context.LastSelectedTimeline) && context.SelectedTimeline == openTimeline;
}
} // namespace ImGui

ImGuiNeoSequencerStyle::ImGuiNeoSequencerStyle() {
	Colors[ImGuiNeoSequencerCol_Bg] = ImVec4{0.31f, 0.31f, 0.31f, 1.00f};
	Colors[ImGuiNeoSequencerCol_TopBarBg] = ImVec4{0.22f, 0.22f, 0.22f, 0.84f};
	Colors[ImGuiNeoSequencerCol_SelectedTimeline] = ImVec4{0.98f, 0.706f, 0.322f, 0.88f};
	Colors[ImGuiNeoSequencerCol_TimelinesBg] = Colors[ImGuiNeoSequencerCol_TopBarBg];
	Colors[ImGuiNeoSequencerCol_TimelineBorder] = Colors[ImGuiNeoSequencerCol_Bg] * ImVec4{0.5f, 0.5f, 0.5f, 1.0f};

	Colors[ImGuiNeoSequencerCol_FramePointer] = ImVec4{0.98f, 0.24f, 0.24f, 0.50f};
	Colors[ImGuiNeoSequencerCol_FramePointerHovered] = ImVec4{0.98f, 0.15f, 0.15f, 1.00f};
	Colors[ImGuiNeoSequencerCol_FramePointerPressed] = ImVec4{0.98f, 0.08f, 0.08f, 1.00f};

	Colors[ImGuiNeoSequencerCol_Keyframe] = ImVec4{0.59f, 0.59f, 0.59f, 0.50f};
	Colors[ImGuiNeoSequencerCol_KeyframeHovered] = ImVec4{0.98f, 0.39f, 0.36f, 1.00f};
	Colors[ImGuiNeoSequencerCol_KeyframePressed] = ImVec4{0.98f, 0.39f, 0.36f, 1.00f};

	Colors[ImGuiNeoSequencerCol_FramePointerLine] = ImVec4{0.98f, 0.98f, 0.98f, 0.8f};

	Colors[ImGuiNeoSequencerCol_ZoomBarBg] = ImVec4{0.59f, 0.59f, 0.59f, 0.90f};
	Colors[ImGuiNeoSequencerCol_ZoomBarSlider] = ImVec4{0.8f, 0.8f, 0.8f, 0.60f};
	Colors[ImGuiNeoSequencerCol_ZoomBarSliderHovered] = ImVec4{0.98f, 0.98f, 0.98f, 0.80f};
	Colors[ImGuiNeoSequencerCol_ZoomBarSliderEnds] = ImVec4{0.59f, 0.59f, 0.59f, 0.90f};
	Colors[ImGuiNeoSequencerCol_ZoomBarSliderEndsHovered] = ImVec4{0.93f, 0.93f, 0.93f, 0.93f};
}
