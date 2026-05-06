//
// Created by Matty on 2022-01-28.
//
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#endif

#include "imgui_neo_sequencer.h"
#include "imgui_internal.h"
#include "imgui_neo_internal.h"

namespace ImGui {

// Internal state, used for deletion of old keyframes.
struct ImGuiNeoTimelineKeyframes {
	ImGuiID TimelineID;
	ImVector<int32_t> KeyframesToDelete;
};

// Internal struct holding how many times was keyframe on certain frame rendered, used as offset for duplicates
struct ImGuiNeoKeyframeDuplicate {
	int32_t Frame;
	uint32_t Count;
};

enum class SelectionState {
	Idle,	   // Doing nothing related
	Selecting, // Selecting selection
	Dragging   // Dragging selection
};

struct ImGuiNeoSequencerInternalData {
	~ImGuiNeoSequencerInternalData() {
		for (ImGuiNeoTimelineKeyframes &keyframes : SelectionData) {
			keyframes.KeyframesToDelete.clear();
		}
		SelectionData.clear_destruct();
	}
	ImVec2 TopLeftCursor = {0, 0};	   // Cursor on top of whole widget
	ImVec2 TopBarStartCursor = {0, 0}; // Cursor on top, below Zoom slider
	float StartValuesX = 0.0f;        // X position for keyframe values

	ImVec2 Size = {0, 0};		// Size of whole sequencer
	ImVec2 TopBarSize = {0, 0}; // Size of top bar without Zoom

	FrameIndexType StartFrame = 0;
	FrameIndexType EndFrame = 0;
	FrameIndexType OffsetFrame = 0; // Offset from start

	float ValuesWidth = 32.0f; // Width of biggest label in timeline, used for offset of timeline

	float FilledHeight = 0.0f; // Height of whole sequencer

	float Zoom = 1.0f;

	ImGuiID Id;

	ImGuiID LastSelectedTimeline = 0;
	ImGuiID SelectedTimeline = 0;
	bool LastTimelineOpenned = false;

	ImVector<ImGuiID> TimelineStack;
	ImVector<ImGuiID> GroupStack;

	FrameIndexType CurrentFrame = 0;
	ImVec4 CurrentFrameColor; // Color of current frame, we have to save it because we render on EndNeoSequencer, but
							  // process at BeginneoSequencer

	bool HoldingZoomSlider = false;

	// Selection
	ImVector<ImGuiID> Selection; // Contains ids of keyframes
	ImVec2 SelectionMouseStart = {0, 0};
	SelectionState StateOfSelection = SelectionState::Idle;
	ImVec2 DraggingMouseStart = {0, 0};
	bool StartDragging = true;
	ImVector<int32_t> DraggingSelectionStart; // Contains start values of all selection elements
	bool DraggingEnabled = true;
	bool SelectionEnabled = true;
	bool IsSelectionRightClicked = false;

	// Last keyframe data
	bool IsLastKeyframeHovered = false;
	bool IsLastKeyframeSelected = false;
	bool IsLastKeyframeRightClicked = false;

	// Deletion
	bool DeleteDataDirty = false;
	bool DeleteEnabled = true;
	ImVector<ImGuiNeoTimelineKeyframes> SelectionData;
};

struct ImGuiNeoSequencerContext {
	ImGuiNeoSequencerStyle Style;
	ImPool<ImGuiNeoSequencerInternalData> Sequencers;
	ImGuiNeoSequencerInternalData *CurrentSequencer = nullptr;
	bool InSequencer = false;
	float CurrentTimelineHeight = 0.0f;
	float CurrentTimelineY = 0.0f;
	uint32_t CurrentTimelineDepth = 0;
	ImVector<ImGuiColorMod> ColorStack;
	ImVector<ImGuiNeoKeyframeDuplicate> KeyframeDuplicates;
};

static ImGuiNeoSequencerContext GNeoSequencerContext;
static ImGuiNeoSequencerContext *GNeoSeq = &GNeoSequencerContext;

///////////// STATIC HELPERS ///////////////////////

static float GetPerFrameWidth(ImGuiNeoSequencerInternalData &context) {
	return GetPerFrameWidth(context.Size.x, context.ValuesWidth, context.EndFrame, context.StartFrame, context.Zoom);
}

static float GetKeyframePositionX(FrameIndexType frame, ImGuiNeoSequencerInternalData &context) {
	const float perFrameWidth = GetPerFrameWidth(context);
	return (float)(frame - context.OffsetFrame - context.StartFrame) * perFrameWidth;
}

static float GetWorkTimelineWidth(ImGuiNeoSequencerInternalData &context) {
	const float perFrameWidth = GetPerFrameWidth(context);
	return context.Size.x - context.ValuesWidth - perFrameWidth;
}

// Dont pull frame from context, its used for dragging
static ImRect GetCurrentFrameBB(FrameIndexType frame, ImGuiNeoSequencerInternalData &context) {
	const ImGuiStyle &imStyle = GetStyle();
	const float width = GNeoSeq->Style.CurrentFramePointerSize * GetStyle().FontScaleMain;
	const ImVec2 cursor =
		context.TopBarStartCursor + ImVec2{context.ValuesWidth + imStyle.FramePadding.x - width / 2.0f, 0};
	const ImVec2 currentFrameCursor = cursor + ImVec2{GetKeyframePositionX(frame, context), 0};

	float pointerHeight = GNeoSeq->Style.CurrentFramePointerSize * 2.5f;
	ImRect rect{currentFrameCursor, currentFrameCursor + ImVec2{width, pointerHeight * GetStyle().FontScaleMain}};

	return rect;
}

static void ProcessCurrentFrame(FrameIndexType *frame, ImGuiNeoSequencerInternalData &context) {
	ImRect pointerRect = GetCurrentFrameBB(*frame, context);
	pointerRect.Min -= ImVec2{2.0f, 2.0f};
	pointerRect.Max += ImVec2{2.0f, 2.0f};

	const ImGuiStyle &imStyle = GetStyle();

	ImGuiID id = GetCurrentWindow()->GetID("currentframeselector");
	if (!ItemAdd(pointerRect, id)) {
		return;
	}

	bool hovered, held;
	/* bool pressed = */ ButtonBehavior(pointerRect, id, &hovered, &held, 0);

	if (held) {
		const float timelineXmin = context.TopBarStartCursor.x + context.ValuesWidth + imStyle.FramePadding.x;
		const ImVec2 timelineXRange = {timelineXmin, timelineXmin + context.Size.x - context.ValuesWidth};
		const float mousePosX = GetMousePos().x;
		// Subtract min
		const float v = mousePosX - timelineXRange.x;
		// Divide by width to remap to 0 - 1 range
		const float normalized = v / GetWorkTimelineWidth(context);
		const float clamped = ImClamp(normalized, 0.0f, 1.0f);
		const float viewSize = (float)(context.EndFrame - context.StartFrame) / context.Zoom;
		const float frameViewVal = (float)context.StartFrame + (clamped * (float)viewSize);
		const FrameIndexType finalFrame = (FrameIndexType)round(frameViewVal) + context.OffsetFrame;

		context.CurrentFrameColor = GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_FramePointerPressed);

		*frame = finalFrame;
	} else if (hovered) {
		context.CurrentFrameColor = GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_FramePointerHovered);
	} else {
		context.CurrentFrameColor = GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_FramePointer);
	}

	context.CurrentFrame = *frame;
}

static ImColor GetKeyframeColor(ImGuiNeoSequencerInternalData &context, bool hovered, bool inSelection) {
	if (inSelection) {
		return ColorConvertFloat4ToU32(GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_KeyframeSelected));
	}

	return hovered ? ColorConvertFloat4ToU32(GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_KeyframeHovered))
				   : ColorConvertFloat4ToU32(GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_Keyframe));
}

static void AddKeyframeToDeleteData(int32_t value, ImGuiNeoSequencerInternalData &context, const ImGuiID timelineId) {
	bool foundTimeline = false;
	for (ImGuiNeoTimelineKeyframes &val : context.SelectionData) {
		if (val.TimelineID == timelineId) {
			foundTimeline = true;
			if (!val.KeyframesToDelete.contains(value))
				val.KeyframesToDelete.push_back(value);
			break;
		}
	}

	if (!foundTimeline) {
		context.SelectionData.push_back({});
		ImGuiNeoTimelineKeyframes &data = context.SelectionData.back();
		data.TimelineID = timelineId;
		data.KeyframesToDelete.push_back(value);
	}
}

static bool GetKeyframeInSelection(int32_t value, ImGuiID id, ImGuiNeoSequencerInternalData &context, const ImRect bb) {
	// TODO(matej.vrba): This is kinda slow, it works for smaller data sample, but for bigger sample it should be
	// changed to hashset
	const ImGuiID timelineId = context.TimelineStack.back();

	if (context.DeleteDataDirty && context.Selection.contains(id)) {
		AddKeyframeToDeleteData(value, context, timelineId);
	}

	if (context.StateOfSelection != SelectionState::Selecting) {
		return context.Selection.contains(id);
	}

	ImRect sel = {context.SelectionMouseStart, GetMousePos()};

	if (sel.Min.y > sel.Max.y) {
		ImVec2 tmp = sel.Min;
		sel.Min = sel.Max;
		sel.Max = tmp;
	}

	if (sel.Min.x > sel.Max.x) {
		float tmp = sel.Min.x;
		sel.Min.x = sel.Max.x;
		sel.Max.x = tmp;
	}

	const bool overlaps = bb.Overlaps(sel);

	const bool forceRemove = IsKeyDown(GNeoSeq->Style.ModRemoveKey);
	const bool forceAdd = IsKeyDown(GNeoSeq->Style.ModAddKey);

	auto removeKeyframe = [&]() {
		for (ImGuiNeoTimelineKeyframes &val : context.SelectionData) {
			if (val.TimelineID == timelineId) {
				val.KeyframesToDelete.find_erase(value);
				break;
			}
		}
		context.Selection.find_erase(id);
	};

	if (overlaps) {
		if (forceRemove) {
			removeKeyframe();
			return context.Selection.contains(id);
		}
		if (!context.Selection.contains(id)) {
			AddKeyframeToDeleteData(value, context, timelineId);

			context.Selection.push_back(id);
		}
	} else {
		if (!forceRemove && !forceAdd) {
			removeKeyframe();
		}
		return context.Selection.contains(id);
	}
	return overlaps;
}

static ImGuiID GetKeyframeID(int32_t *frame) {
	return GetCurrentWindow()->GetID(frame);
}

static bool CreateKeyframe(int32_t *frame) {
	const ImGuiStyle &imStyle = GetStyle();
	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;

	const float timelineOffset = GetKeyframePositionX(*frame, context);
	float offset = 0.0f;

	for (ImGuiNeoKeyframeDuplicate &duplicateData : GNeoSeq->KeyframeDuplicates) {
		if (duplicateData.Frame == *frame) {
			offset = (float)duplicateData.Count * GNeoSeq->Style.CollidedKeyframeOffset;
			duplicateData.Count++;
		}
	}

	if (offset < GNeoSeq->Style.CollidedKeyframeOffset) {
		GNeoSeq->KeyframeDuplicates.push_back({});
		GNeoSeq->KeyframeDuplicates.back().Frame = *frame;
		GNeoSeq->KeyframeDuplicates.back().Count = 1;
	}

	const ImVec2 pos = ImVec2{context.StartValuesX + imStyle.FramePadding.x, GNeoSeq->CurrentTimelineY} +
					   ImVec2{timelineOffset + context.ValuesWidth + offset, 0};
	const ImVec2 bbPos = pos - ImVec2{GNeoSeq->CurrentTimelineHeight / 2, 0};
	const ImRect bb = {bbPos, bbPos + ImVec2{GNeoSeq->CurrentTimelineHeight, GNeoSeq->CurrentTimelineHeight}};
	const ImGuiID id = GetKeyframeID(frame);
	if (!ItemAdd(bb, id)) {
		return false;
	}
	bool hovered, held;
	/*bool pressed =*/ButtonBehavior(bb, id, &hovered, &held, 0);

	if (context.SelectionEnabled && context.Selection.contains(id) &&
		(context.StateOfSelection != SelectionState::Selecting)) {
		// process dragging
		if (held && hovered && context.StateOfSelection != SelectionState::Dragging && context.DraggingEnabled) {
			// Start dragging
			context.StartDragging = true;
		}

		if (context.StateOfSelection == SelectionState::Dragging) {
			ImGuiID *it = context.Selection.find(id);
			int32_t index = context.Selection.index_from_ptr(it);

			if (context.DraggingSelectionStart.size() < index + 1 || context.DraggingSelectionStart[index] == -1) {
				if (context.DraggingSelectionStart.size() < index + 1) {
					context.DraggingSelectionStart.resize(index + 1, -1);
				}

				context.DraggingSelectionStart[index] = *frame;
			}
			const float mouseDelta = GetMousePos().x - context.DraggingMouseStart.x;
			const float viewSize = (float)(context.EndFrame - context.StartFrame) / context.Zoom;
			const int32_t offsetA = int32_t(mouseDelta / (context.Size.x / viewSize));
			*frame = context.DraggingSelectionStart[index] + offsetA;
		}
	}

	const bool inSelection = GetKeyframeInSelection(*frame, id, context, bb);

	context.IsLastKeyframeSelected = inSelection;

	if (timelineOffset >= 0.0f) {
		const ImColor color = GetKeyframeColor(context, hovered, inSelection);
		ImDrawList *drawList = ImGui::GetWindowDrawList();
		drawList->AddCircleFilled(pos + ImVec2{0, GNeoSeq->CurrentTimelineHeight / 2.f}, GNeoSeq->CurrentTimelineHeight / 3.0f, color, 4);
	}

	context.IsLastKeyframeHovered = hovered;
	context.IsLastKeyframeRightClicked = hovered && IsMouseClicked(ImGuiMouseButton_Right);

	if (context.Selection.contains(id) && context.IsLastKeyframeRightClicked) {
		context.IsSelectionRightClicked = true;
	}

	return true;
}

static void RenderCurrentFrame(ImGuiNeoSequencerInternalData &context) {
	const ImRect bb = GetCurrentFrameBB(context.CurrentFrame, context);
	ImDrawList *drawList = ImGui::GetWindowDrawList();

	RenderNeoSequencerCurrentFrame(GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_FramePointerLine),
								   context.CurrentFrameColor, bb, context.Size.y - context.TopBarSize.y,
								   GNeoSeq->Style.CurrentFrameLineWidth, drawList);
}

static float CalculateZoomBarHeight() {
	const ImGuiStyle &imStyle = GetStyle();
	return GetFontSize() * GNeoSeq->Style.ZoomHeightScale + imStyle.FramePadding.y * 2.0f;
}

static void ProcessAndRenderZoom(ImGuiNeoSequencerInternalData &context, const ImVec2 &cursor, bool allowEditingLength,
								 FrameIndexType *start, FrameIndexType *end) {
	const ImGuiStyle &imStyle = GetStyle();
	ImGuiWindow *window = GetCurrentWindow();

	const float zoomHeight = CalculateZoomBarHeight();

	ImDrawList *drawList = GetWindowDrawList();

	// Input width
	const float inputWidth = CalcTextSize("123456").x;
	const float inputWidthWithPadding = inputWidth + imStyle.ItemSpacing.x;
	const ImVec2 cursorV = allowEditingLength ? cursor + ImVec2{inputWidthWithPadding, 0} : cursor;
	const float size = allowEditingLength ? context.Size.x - 2 * inputWidthWithPadding : context.Size.x;
	const ImRect bb{cursorV, cursorV + ImVec2{size, zoomHeight}};
	const ImVec2 zoomBarEndWithSpacing = ImVec2{bb.Max.x + imStyle.ItemSpacing.x, bb.Min.y};
	FrameIndexType startFrameVal = *start;
	FrameIndexType endFrameVal = *end;

	if (allowEditingLength) {
		const float sideOffset = imStyle.ItemSpacing.x / 2.0f;
		ImVec2 prevWindowCursor = window->DC.CursorPos;
		ImVec2 prevCursorMaxPos = window->DC.CursorMaxPos;

		window->DC.CursorPos = cursor;

		window->DC.CursorPos.x += sideOffset;

		PushItemWidth(inputWidth);
		InputScalar("##input_start_frame", ImGuiDataType_U32, &startFrameVal, NULL, NULL, "%i",
					allowEditingLength ? 0 : ImGuiInputTextFlags_ReadOnly);

		window->DC.CursorPos = ImVec2{zoomBarEndWithSpacing.x, cursor.y};
		window->DC.CursorPos.x -= sideOffset;

		PushItemWidth(inputWidth);
		InputScalar("##input_end_frame", ImGuiDataType_U32, &endFrameVal, NULL, NULL, "%i",
					allowEditingLength ? 0 : ImGuiInputTextFlags_ReadOnly);

		window->DC.CursorPos = prevWindowCursor;
		window->DC.CursorMaxPos = prevCursorMaxPos;
	}

	if (endFrameVal <= startFrameVal) {
		endFrameVal = (int32_t)*end;
	}

	*start = startFrameVal;
	*end = endFrameVal;

	// Background
	drawList->AddRectFilled(
		bb.Min, bb.Max, ColorConvertFloat4ToU32(GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_ZoomBarBg)), 10.0f);

	const float baseWidth =
		bb.GetSize().x -
		imStyle.ItemInnerSpacing.x; // There is just half spacing applied, doing it normally makes big gap on sides

	const float sliderHeight = bb.GetSize().y - imStyle.ItemInnerSpacing.y;
	const float sliderWidth = baseWidth / context.Zoom;
	const ImVec2 sliderMin = bb.Min + imStyle.ItemInnerSpacing / 2.0f;
	const float sliderMaxWidth = baseWidth;
	const FrameIndexType totalFrames = (*end - *start);
	const float singleFrameWidthOffset = sliderMaxWidth / (float)totalFrames;
	const float zoomSliderOffset = singleFrameWidthOffset * (float)context.OffsetFrame;
	const ImVec2 sliderStart = sliderMin + ImVec2{zoomSliderOffset, 0};
	const float sideSize = sliderHeight;
	const ImRect finalSliderBB{sliderStart, sliderStart + ImVec2{sliderWidth, sliderHeight}};
	const ImRect finalSliderInteractBB = {finalSliderBB.Min + ImVec2{sideSize, 0},
										  finalSliderBB.Max - ImVec2{sideSize, 0}};
	const uint32_t viewWidth = (uint32_t)((float)totalFrames / context.Zoom);
	// TODO: use SliderBehavior
	const bool hovered = ItemHoverable(bb, GetCurrentWindow()->GetID("##zoom_slider"), 0);

	if (hovered) {
		SetKeyOwner(ImGuiKey_MouseWheelY, GetItemID());
		const float currentScroll = GetIO().MouseWheel;

		context.Zoom = ImClamp(context.Zoom + float(currentScroll) * 0.3f, 1.0f, (float)viewWidth);
		const FrameIndexType newZoomWidth = (FrameIndexType)ceil((float)totalFrames / (context.Zoom));

		if (*start + context.OffsetFrame + newZoomWidth > *end)
			context.OffsetFrame = ImMax(0U, totalFrames - viewWidth);
	}

	if (context.HoldingZoomSlider) {
		if (IsMouseDragging(ImGuiMouseButton_Left, 0.01f)) {
			const float currentX = GetMousePos().x;
			// Subtract min
			const float v = currentX - bb.Min.x;
			// Divide by width to remap to 0 - 1 range
			const float normalized = v / bb.GetWidth();
			const float sliderWidthNormalized = 1.0f / context.Zoom;
			const float singleFrameWidthOffsetNormalized = singleFrameWidthOffset / bb.GetWidth();

			FrameIndexType finalFrame =
				(FrameIndexType)((float)(normalized - sliderWidthNormalized / 2.0f) / singleFrameWidthOffsetNormalized);

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

	const bool res = ItemAdd(finalSliderInteractBB, 0);

	const FrameIndexType viewStart = *start + context.OffsetFrame;
	const uint32_t viewEnd = viewStart + viewWidth;

	if (res) {
		ImVec4 sliderColor = GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_ZoomBarSlider);

		if (IsItemHovered()) {
			sliderColor = GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_ZoomBarSliderHovered);
		}

		// Render bar
		drawList->AddRectFilled(finalSliderBB.Min, finalSliderBB.Max, ColorConvertFloat4ToU32(sliderColor), 10.0f);

		const ImVec2 sliderCenter = finalSliderBB.GetCenter();

		char overlayTextBuffer[128];
		snprintf(overlayTextBuffer, sizeof(overlayTextBuffer), "%i - %i", viewStart, viewEnd);

		const ImVec2 overlaySize = CalcTextSize(overlayTextBuffer);
		drawList->AddText(sliderCenter - overlaySize / 2.0f, IM_COL32_WHITE, overlayTextBuffer);
	}
}

static void ProcessSelection(ImGuiNeoSequencerInternalData &context) {
	context.DeleteDataDirty = false;

	if (context.StartDragging) {
		context.StateOfSelection = SelectionState::Dragging;
		context.DraggingMouseStart = GetMousePos();
		context.StartDragging = false;
		return;
	}

	const ImRect windowWorkRect = GetCurrentWindow()->ClipRect;

	const ImRect sequencerWorkRect =
		ImRect{context.TopBarStartCursor + ImVec2{context.ValuesWidth, context.TopBarSize.y},
			   context.TopBarStartCursor + context.Size - ImVec2{0, context.TopBarSize.y}};

	const bool mouseInWorkArea = windowWorkRect.Contains(GetMousePos()) &&
		sequencerWorkRect.Contains(GetMousePos());

	if (IsMouseDown(ImGuiMouseButton_Left) &&
		(mouseInWorkArea || context.StateOfSelection != SelectionState::Idle)) {
		// Keep selecting/dragging even if mouse leaves the window while button is held
		switch (context.StateOfSelection) {
		case SelectionState::Idle: {
			if (!IsMouseClicked(ImGuiMouseButton_Left))
				return;

			context.SelectionMouseStart = GetMousePos();
			context.StateOfSelection = SelectionState::Selecting;
			break;
		}
		case SelectionState::Selecting: {
		case SelectionState::Dragging:
			break;
		}
		}
	} else {
		switch (context.StateOfSelection) {
		case SelectionState::Idle: {
			break;
		}
		case SelectionState::Selecting: {
			context.SelectionMouseStart = {0, 0};
			context.StateOfSelection = SelectionState::Idle;
			break;
		}
		case SelectionState::Dragging: {
			context.DraggingSelectionStart.resize(0);
			context.StateOfSelection = SelectionState::Idle;
			context.DraggingMouseStart = {0, 0};
			context.DeleteDataDirty = true;
			for (ImGuiNeoTimelineKeyframes &t : context.SelectionData) {
				t.KeyframesToDelete.resize(0);
			}
			break;
		}
		}
	}
}

static void RenderSelection(ImGuiNeoSequencerInternalData &context) {
	if (context.StateOfSelection != SelectionState::Selecting) {
		return;
	}
	const ImVec2 currentMousePosition = GetMousePos();
	ImDrawList *drawList = GetWindowDrawList();
	ImRect sel{context.SelectionMouseStart, currentMousePosition};

	if (sel.Min.y > sel.Max.y) {
		ImVec2 tmp = sel.Min;
		sel.Min = sel.Max;
		sel.Max = tmp;
	}

	if (sel.Min.x > sel.Max.x) {
		float tmp = sel.Min.x;
		sel.Min.x = sel.Max.x;
		sel.Max.x = tmp;
	}

	if (sel.GetArea() < 32.0f) {
		return;
	}

	// Inner
	drawList->AddRectFilled(context.SelectionMouseStart, currentMousePosition,
							ColorConvertFloat4ToU32(GNeoSeq->Style.Colors[ImGuiNeoSequencerCol_Selection]));

	// border
	drawList->AddRect(context.SelectionMouseStart, currentMousePosition,
					  ColorConvertFloat4ToU32(GNeoSeq->Style.Colors[ImGuiNeoSequencerCol_SelectionBorder]), 0.0f, 0, 0.5f);
}

static bool GroupBehaviour(const ImGuiID id, bool *open, const ImVec2 labelSize) {
	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;
	ImGuiWindow *window = GetCurrentWindow();

	const bool closable = open != nullptr;

	ImDrawList *drawList = ImGui::GetWindowDrawList();
	const float arrowWidth = drawList->_Data->FontSize;
	const ImVec2 arrowSize = {arrowWidth, arrowWidth};
	const ImVec2 cursor = {context.TopBarStartCursor.x, window->DC.CursorPos.y};
	ItemSize(ImVec2{0, labelSize.y});
	const ImRect arrowBB = {cursor, cursor + arrowSize};
	const ImVec2 groupBBMin = {cursor + ImVec2{arrowSize.x, 0.0f}};
	const ImRect groupBB = {groupBBMin, groupBBMin + labelSize};
	const ImGuiID arrowID = window->GetID("##arrow");
	const bool addArrowRes = ItemAdd(arrowBB, arrowID);
	if (addArrowRes) {
		if (IsItemClicked() && closable) {
			(*open) = !(*open);
		}
	}

	const bool addGroupRes = ItemAdd(groupBB, id);
	if (addGroupRes) {
		if (IsItemClicked()) {
			context.LastSelectedTimeline = context.SelectedTimeline;
			context.SelectedTimeline = id;
		}
	}
	const float width = groupBB.Max.x - arrowBB.Min.x;
	context.ValuesWidth = ImMax(context.ValuesWidth, width); // Make left panel wide enough
	return addGroupRes && addArrowRes;
}

static bool TimelineBehaviour(const ImGuiID id, const ImVec2 labelSize) {
	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;
	ImGuiWindow *window = GetCurrentWindow();
	const ImVec2 cursor = {context.TopBarStartCursor.x, window->DC.CursorPos.y};
	ItemSize(ImVec2{0, labelSize.y});
	const ImRect groupBB = {cursor, cursor + labelSize};
	const bool addGroupRes = ItemAdd(groupBB, id);
	if (addGroupRes) {
		if (IsItemClicked()) {
			context.LastSelectedTimeline = context.SelectedTimeline;
			context.SelectedTimeline = id;
		}
	}
	const float width = groupBB.Max.x - groupBB.Min.x;
	context.ValuesWidth = ImMax(context.ValuesWidth, width);

	return addGroupRes;
}

////////////////////////////////////

const ImVec4 &GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol idx) {
	return GetNeoSequencerStyle().Colors[idx];
}

ImGuiNeoSequencerStyle &GetNeoSequencerStyle() {
	return GNeoSeq->Style;
}

bool BeginNeoSequencer(const char *idin, FrameIndexType *frame, FrameIndexType *startFrame, FrameIndexType *endFrame,
					   const ImVec2 &size, ImGuiNeoSequencerFlags flags) {
	IM_ASSERT(!GNeoSeq->InSequencer && "Called when while in other NeoSequencer, that won't work, call End!");
	IM_ASSERT(*startFrame < *endFrame && "Start frame must be smaller than end frame");

	static char childNameStorage[64];
	snprintf(childNameStorage, sizeof(childNameStorage), "##%s_child_wrapper", idin);
	const bool openChild = BeginChild(childNameStorage);

	if (!openChild) {
		EndChild();
		return openChild;
	}

	if (GNeoSeq->InSequencer) {
		return false;
	}

	ImGuiWindow *window = GetCurrentWindow();
	if (window->SkipItems) {
		return false;
	}

	const ImVec2 cursor = GetCursorScreenPos();
	const ImVec2 area = ImGui::GetContentRegionAvail();

	const ImVec2 cursorBasePos = cursor + window->Scroll;

	PushID(idin);
	const unsigned int id = window->IDStack[window->IDStack.size() - 1];

	GNeoSeq->InSequencer = true;

	ImGuiNeoSequencerInternalData *sequencer = GNeoSeq->Sequencers.GetOrAddByKey(id);
	GNeoSeq->CurrentSequencer = sequencer;
	ImGuiNeoSequencerInternalData &context = *sequencer;
	context.Id = id;

	ImVec2 realSize = ImFloor(size);
	if (realSize.x <= 0.0f)
		realSize.x = ImMax(4.0f, area.x);
	if (realSize.y <= 0.0f)
		realSize.y = ImMax(4.0f, context.FilledHeight);

	const bool showZoom = !(flags & ImGuiNeoSequencerFlags_HideZoom);
	const bool headerAlwaysVisible = (flags & ImGuiNeoSequencerFlags_AlwaysShowHeader);
	context.SelectionEnabled = (flags & ImGuiNeoSequencerFlags_EnableSelection);
	context.DraggingEnabled = context.SelectionEnabled && (flags & ImGuiNeoSequencerFlags_Selection_EnableDragging);
	context.DeleteEnabled = context.SelectionEnabled && (flags & ImGuiNeoSequencerFlags_Selection_EnableDeletion);

	context.TopLeftCursor = headerAlwaysVisible ? cursorBasePos : cursor;

	// If Zoom is shown, we offset it by height of Zoom bar + padding
	context.TopBarStartCursor =
		showZoom ? context.TopLeftCursor + ImVec2{0, CalculateZoomBarHeight()} : context.TopLeftCursor;
	context.StartFrame = *startFrame;
	context.EndFrame = *endFrame;
	context.Size = realSize;

	context.TopBarSize = ImVec2(context.Size.x, GNeoSeq->Style.TopBarHeight);

	if (context.TopBarSize.y <= 0.0f) {
		const ImGuiStyle &imStyle = GetStyle();
		context.TopBarSize.y = CalcTextSize("100").y + imStyle.FramePadding.y * 2.0f;
	}


	ImVec2 backgroundSize = context.Size;
	const float topCut = abs(context.TopLeftCursor.y - cursor.y);
	backgroundSize.y = backgroundSize.y - (topCut);

	ImDrawList *drawList = GetWindowDrawList();
	RenderNeoSequencerBackground(GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_Bg), context.TopLeftCursor,
								 backgroundSize, drawList, GNeoSeq->Style.SequencerRounding);

	RenderNeoSequencerTopBarBackground(GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_TopBarBg),
									   context.TopBarStartCursor, context.TopBarSize, drawList,
									   GNeoSeq->Style.SequencerRounding);

	RenderNeoSequencerTopBarOverlay(context.Zoom, context.ValuesWidth, context.StartFrame, context.EndFrame,
									context.OffsetFrame, context.TopBarStartCursor, context.TopBarSize, drawList,
									GNeoSeq->Style.TopBarShowFrameLines, GNeoSeq->Style.TopBarShowFrameTexts, GNeoSeq->Style.MaxSizePerTick);

	if (showZoom) {
		ProcessAndRenderZoom(context, context.TopLeftCursor, flags & ImGuiNeoSequencerFlags_AllowLengthChanging,
							 startFrame, endFrame);
	}

	if (context.Size.y < context.FilledHeight) {
		context.Size.y = context.FilledHeight;
	}

	context.FilledHeight = context.TopBarSize.y + GNeoSeq->Style.TopBarSpacing + (showZoom ? CalculateZoomBarHeight() : 0.0f);

	context.StartValuesX = cursor.x;
	float startValuesY = cursor.y + context.TopBarSize.y + GNeoSeq->Style.TopBarSpacing;
	if (showZoom) {
		startValuesY += CalculateZoomBarHeight();
	}
	window->DC.CursorPos.y = startValuesY;

	ProcessCurrentFrame(frame, context);

	// if (enableSelection)
	// processSelection(context);

	const ImVec2 clipMin = context.TopBarStartCursor + ImVec2(0, context.TopBarSize.y);

	drawList->PushClipRect(clipMin,
						   clipMin + backgroundSize - ImVec2(0, context.TopBarSize.y) -
							   ImVec2{0, GetFontSize() * GNeoSeq->Style.ZoomHeightScale},
						   true);

	return true;
}

void EndNeoSequencer() {
	IM_ASSERT(GNeoSeq->InSequencer && "Called end sequencer when BeginSequencer didnt return true or wasn't called at all!");
	IM_ASSERT(GNeoSeq->CurrentSequencer != nullptr && "Ended sequencer has no context!");

	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;
	IM_ASSERT(context.TimelineStack.empty() && "Missmatch in timeline Begin / End");

	if (context.SelectionEnabled)
		ProcessSelection(context);

	context.LastSelectedTimeline = context.SelectedTimeline;
	context.IsSelectionRightClicked = false;

	if (context.SelectionEnabled) {
		RenderSelection(context);
	}

	RenderCurrentFrame(context);

	GNeoSeq->InSequencer = false;
	GNeoSeq->CurrentSequencer = nullptr;

	ImGuiWindow *window = GetCurrentWindow();
	context.FilledHeight = window->DC.CursorPos.y - window->DC.CursorStartPos.y;
	context.Size.y = context.FilledHeight;

	PopID();

	EndChild();
}

bool BeginNeoGroup(const char *label, bool *open) {
	return BeginNeoTimeline(label, nullptr, 0, open, ImGuiNeoTimelineFlags_Group);
}

void EndNeoGroup() {
	return EndNeoTimeLine();
}

void PushNeoSequencerStyleColor(ImGuiNeoSequencerCol idx, ImU32 col) {
	ImGuiColorMod backup;
	backup.Col = idx;
	backup.BackupValue = GNeoSeq->Style.Colors[idx];
	GNeoSeq->ColorStack.push_back(backup);
	GNeoSeq->Style.Colors[idx] = ColorConvertU32ToFloat4(col);
}

void PushNeoSequencerStyleColor(ImGuiNeoSequencerCol idx, const ImVec4 &col) {
	ImGuiColorMod backup;
	backup.Col = idx;
	backup.BackupValue = GNeoSeq->Style.Colors[idx];
	GNeoSeq->ColorStack.push_back(backup);
	GNeoSeq->Style.Colors[idx] = col;
}

void PopNeoSequencerStyleColor(int count) {
	while (count > 0) {
		ImGuiColorMod &backup = GNeoSeq->ColorStack.back();
		GNeoSeq->Style.Colors[backup.Col] = backup.BackupValue;
		GNeoSeq->ColorStack.pop_back();
		count--;
	}
}

void SetSelectedTimeline(const char *timelineLabel) {
	IM_ASSERT(GNeoSeq->InSequencer && "Not in active sequencer!");

	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;

	ImGuiWindow *window = GetCurrentWindow();

	ImGuiID timelineID = 0;

	if (timelineLabel) {
		timelineID = window->GetID(timelineLabel);
	}
	context.LastSelectedTimeline = context.SelectedTimeline;
	context.SelectedTimeline = timelineID;
}

bool IsNeoTimelineSelected(ImGuiNeoTimelineIsSelectedFlags flags) {
	IM_ASSERT(GNeoSeq->InSequencer && "Not in active sequencer!");
	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;

	IM_ASSERT(!context.TimelineStack.empty() && "No active timelines are present!");

	const bool newly = flags & ImGuiNeoTimelineIsSelectedFlags_NewlySelected;

	const unsigned int openTimeline = context.TimelineStack[context.TimelineStack.size() - 1];

	if (!newly) {
		return context.SelectedTimeline == openTimeline;
	}

	return context.SelectedTimeline != context.LastSelectedTimeline && context.SelectedTimeline == openTimeline;
}

bool BeginNeoTimelineEx(const char *label, bool *open, ImGuiNeoTimelineFlags flags) {
	IM_ASSERT(GNeoSeq->InSequencer && "Not in active sequencer!");

	const bool closable = open != nullptr;

	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;
	const ImGuiStyle &imStyle = GetStyle();
	ImGuiWindow *window = GetCurrentWindow();
	if (window->SkipItems) {
		return false;
	}

	const ImGuiID id = window->GetID(label);
	ImVec2 labelSize = CalcTextSize(label);

	labelSize.y += imStyle.FramePadding.y * 2 + GNeoSeq->Style.ItemSpacing.y * 2;
	labelSize.x +=
		imStyle.FramePadding.x * 2 + GNeoSeq->Style.ItemSpacing.x * 2 + (float)GNeoSeq->CurrentTimelineDepth * GNeoSeq->Style.DepthItemSpacing;

	bool isGroup = flags & ImGuiNeoTimelineFlags_Group && closable;
	bool addRes = false;
	const float itemPosY = window->DC.CursorPos.y;
	if (isGroup) {
		labelSize.x += imStyle.ItemSpacing.x + GetFontSize();
		addRes = GroupBehaviour(id, open, labelSize);
	} else {
		addRes = TimelineBehaviour(id, labelSize);
	}

	GNeoSeq->CurrentTimelineHeight = labelSize.y;
	GNeoSeq->CurrentTimelineY = itemPosY;
	if (!ImGui::IsItemVisible()) {
		GNeoSeq->CurrentTimelineHeight = 0.0f;
		return false;
	}

	const bool result = !closable || (*open);
	context.LastTimelineOpenned = result;

	const ImVec2 cursor = {context.TopBarStartCursor.x, itemPosY};
	if (addRes) {
		RenderNeoTimelane(id == context.SelectedTimeline, cursor + ImVec2{context.ValuesWidth, 0},
						  ImVec2{context.Size.x - context.ValuesWidth, GNeoSeq->CurrentTimelineHeight},
						  GetStyleNeoSequencerColorVec4(ImGuiNeoSequencerCol_SelectedTimeline));

		ImVec4 color = GetStyleColorVec4(ImGuiCol_Text);
		if (IsItemHovered()) {
			color.w *= 0.7f;
		}

		RenderNeoTimelineLabel(label,
							   cursor + imStyle.FramePadding +
								   ImVec2{(float)GNeoSeq->CurrentTimelineDepth * GNeoSeq->Style.DepthItemSpacing, 0},
							   labelSize, color, isGroup, isGroup && (*open));
	}

	if (result)
		context.TimelineStack.push_back(id);

	if (isGroup) { // Group requires special behaviour if its closed
		if (result) {
			GNeoSeq->CurrentTimelineDepth++;
			context.GroupStack.push_back(id);
		}
	}

	GNeoSeq->KeyframeDuplicates.resize(0);

	return result;
}

bool BeginNeoTimeline(const char *label, FrameIndexType **keyframes, uint32_t keyframeCount, bool *open,
					  ImGuiNeoTimelineFlags flags) {
	if (!BeginNeoTimelineEx(label, open, flags)) {
		return false;
	}

	for (uint32_t i = 0; i < keyframeCount; i++) {
		NeoKeyframe(keyframes[i]);
	}

	return true;
}

void EndNeoTimeLine() {
	IM_ASSERT(GNeoSeq->InSequencer && "Not in active sequencer!");

	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;

	IM_ASSERT(context.TimelineStack.size() > 0 && "Timeline stack push/pop missmatch!");

	GNeoSeq->CurrentTimelineHeight = 0.0f;

	if (!context.TimelineStack.empty() && !context.GroupStack.empty() &&
		context.TimelineStack.back() == context.GroupStack.back()) {
		GNeoSeq->CurrentTimelineDepth--;
		context.GroupStack.pop_back();
	}

	context.TimelineStack.pop_back();
}

void NeoKeyframe(int32_t *value) {
	IM_ASSERT(GNeoSeq->InSequencer && "Not in active sequencer!");
	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;
	IM_ASSERT(!context.TimelineStack.empty() && "Not in timeline!");
	(void)context;

	CreateKeyframe(value);
}

bool IsNeoKeyframeHovered() {
	IM_ASSERT(GNeoSeq->InSequencer && "Not in active sequencer!");
	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;
	return context.IsLastKeyframeHovered;
}

bool IsNeoKeyframeSelected() {
	IM_ASSERT(GNeoSeq->InSequencer && "Not in active sequencer!");
	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;
	return context.IsLastKeyframeSelected;
}

bool IsNeoKeyframeRightClicked() {
	IM_ASSERT(GNeoSeq->InSequencer && "Not in active sequencer!");
	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;
	return context.IsLastKeyframeRightClicked;
}

void NeoClearSelection() {
	IM_ASSERT(GNeoSeq->InSequencer && "Not in active sequencer!");
	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;
	context.Selection.resize(0);
	context.SelectionData.resize(0);
}

bool NeoIsSelecting() {
	IM_ASSERT(GNeoSeq->InSequencer && "Not in active sequencer!");
	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;
	return context.StateOfSelection == SelectionState::Selecting;
}

bool NeoHasSelection() {
	IM_ASSERT(GNeoSeq->InSequencer && "Not in active sequencer!");
	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;
	return !context.Selection.empty();
}

bool NeoIsDraggingSelection() {
	IM_ASSERT(GNeoSeq->InSequencer && "Not in active sequencer!");
	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;
	return context.StateOfSelection == SelectionState::Dragging;
}

uint32_t GetNeoKeyframeSelectionSize() {
	IM_ASSERT(GNeoSeq->InSequencer && "Not in active sequencer!");
	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;

	if (!context.DeleteEnabled) {
		return 0;
	}

	IM_ASSERT(!context.TimelineStack.empty() && "Not in timeline!");
	const ImGuiID timelineId = context.TimelineStack.back();

	for (ImGuiNeoTimelineKeyframes &deleteSelection : context.SelectionData) {
		if (deleteSelection.TimelineID == timelineId) {
			return deleteSelection.KeyframesToDelete.size();
		}
	}

	return 0;
}

void GetNeoKeyframeSelection(FrameIndexType *selection) {
	IM_ASSERT(GNeoSeq->InSequencer && "Not in active sequencer!");
	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;

	if (!context.DeleteEnabled) {
		return;
	}

	IM_ASSERT(!context.TimelineStack.empty() && "Not in timeline!");
	const ImGuiID timelineId = context.TimelineStack.back();

	for (ImGuiNeoTimelineKeyframes &deleteSelection : context.SelectionData) {
		if (deleteSelection.TimelineID == timelineId) {
			for (int32_t i = 0; i < deleteSelection.KeyframesToDelete.size(); i++) {
				selection[i] = deleteSelection.KeyframesToDelete[i];
			}
			return;
		}
	}
}

bool IsNeoKeyframeSelectionRightClicked() {
	IM_ASSERT(GNeoSeq->InSequencer && "Not in active sequencer!");
	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;
	return context.IsSelectionRightClicked;
}

bool NeoCanDeleteSelection() {
	IM_ASSERT(GNeoSeq->InSequencer && "Not in active sequencer!");
	ImGuiNeoSequencerInternalData &context = *GNeoSeq->CurrentSequencer;
	return context.DeleteEnabled && NeoHasSelection() && !NeoIsSelecting() && !NeoIsDraggingSelection();
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
	Colors[ImGuiNeoSequencerCol_KeyframeSelected] = ImVec4{0.32f, 0.23f, 0.98f, 1.00f};

	Colors[ImGuiNeoSequencerCol_FramePointerLine] = ImVec4{0.98f, 0.98f, 0.98f, 0.8f};

	Colors[ImGuiNeoSequencerCol_ZoomBarBg] = ImVec4{0.59f, 0.59f, 0.59f, 0.90f};
	Colors[ImGuiNeoSequencerCol_ZoomBarSlider] = ImVec4{0.8f, 0.8f, 0.8f, 0.60f};
	Colors[ImGuiNeoSequencerCol_ZoomBarSliderHovered] = ImVec4{0.98f, 0.98f, 0.98f, 0.80f};
	Colors[ImGuiNeoSequencerCol_ZoomBarSliderEnds] = ImVec4{0.59f, 0.59f, 0.59f, 0.90f};
	Colors[ImGuiNeoSequencerCol_ZoomBarSliderEndsHovered] = ImVec4{0.93f, 0.93f, 0.93f, 0.93f};

	Colors[ImGuiNeoSequencerCol_SelectionBorder] = ImVec4{0.98f, 0.706f, 0.322f, 0.61f};
	Colors[ImGuiNeoSequencerCol_Selection] = ImVec4{0.98f, 0.706f, 0.322f, 0.33f};
}
