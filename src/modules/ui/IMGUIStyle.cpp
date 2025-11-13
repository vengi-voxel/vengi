/**
 * @file
 * @ingroup UI
 */

#include "IMGUIStyle.h"
#include "dearimgui/imgui.h"
#include "dearimgui/imgui_neo_sequencer.h"
#include "dearimgui/ImGuizmo.h"

namespace ImGui {

// https://github.com/ocornut/imgui/issues/707
void StyleColorsCorporateGrey() {
	ImGuiStyle &style = ImGui::GetStyle();
	ImVec4 *colors = style.Colors;

	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
	colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
	colors[ImGuiCol_Separator] = ImVec4(0.000f, 0.000f, 0.000f, 0.3f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_TabSelected] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
	colors[ImGuiCol_TabSelectedOverline] = colors[ImGuiCol_HeaderActive];
	colors[ImGuiCol_TabDimmed] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
	colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f); // Prefer using Alpha=1.0 here
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);	 // Prefer using Alpha=1.0 here
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
	colors[ImGuiCol_TextLink] = colors[ImGuiCol_HeaderActive];
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_DragDropTargetBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_NavCursor] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void StyleColorsNeoSequencer() {
	const ImGuiStyle &style = ImGui::GetStyle();

	ImGuiNeoSequencerStyle &neoStyle = GetNeoSequencerStyle();

	neoStyle.Colors[ImGuiNeoSequencerCol_Bg] = style.Colors[ImGuiCol_WindowBg];
	neoStyle.Colors[ImGuiNeoSequencerCol_TopBarBg] = style.Colors[ImGuiCol_TableHeaderBg];
	neoStyle.Colors[ImGuiNeoSequencerCol_SelectedTimeline] = style.Colors[ImGuiCol_HeaderHovered];
	neoStyle.Colors[ImGuiNeoSequencerCol_TimelinesBg] = neoStyle.Colors[ImGuiNeoSequencerCol_TopBarBg];
	const ImVec4 &bg = neoStyle.Colors[ImGuiNeoSequencerCol_Bg];
	neoStyle.Colors[ImGuiNeoSequencerCol_TimelineBorder] = ImVec4{bg.x * 0.5f, bg.y * 0.5f, bg.z * 0.5f, 1.0f};

	neoStyle.Colors[ImGuiNeoSequencerCol_FramePointer] = ImVec4{0.98f, 0.24f, 0.24f, 0.50f};
	neoStyle.Colors[ImGuiNeoSequencerCol_FramePointerHovered] = ImVec4{0.98f, 0.15f, 0.15f, 1.00f};
	neoStyle.Colors[ImGuiNeoSequencerCol_FramePointerPressed] = ImVec4{0.98f, 0.08f, 0.08f, 1.00f};

	const ImVec4 &keyframe = style.Colors[ImGuiCol_TextDisabled];
	neoStyle.Colors[ImGuiNeoSequencerCol_Keyframe] = keyframe;
	neoStyle.Colors[ImGuiNeoSequencerCol_KeyframeHovered] = ImVec4(keyframe.x * 0.8f, keyframe.y * 0.8f, keyframe.z * 0.8f, 1.0f);
	neoStyle.Colors[ImGuiNeoSequencerCol_KeyframePressed] = neoStyle.Colors[ImGuiNeoSequencerCol_KeyframeHovered];
	neoStyle.Colors[ImGuiNeoSequencerCol_KeyframeSelected] = style.Colors[ImGuiCol_Text];

	neoStyle.Colors[ImGuiNeoSequencerCol_FramePointerLine] = neoStyle.Colors[ImGuiNeoSequencerCol_TimelineBorder];

	neoStyle.Colors[ImGuiNeoSequencerCol_ZoomBarBg] = style.Colors[ImGuiCol_ScrollbarBg];
	neoStyle.Colors[ImGuiNeoSequencerCol_ZoomBarSlider] = style.Colors[ImGuiCol_ScrollbarGrab];
	neoStyle.Colors[ImGuiNeoSequencerCol_ZoomBarSliderHovered] = style.Colors[ImGuiCol_ScrollbarGrabHovered];
	// unused
	// neoStyle.Colors[ImGuiNeoSequencerCol_ZoomBarSliderEnds] = ImVec4{1.0f, 0.59f, 0.59f, 0.90f};
	// neoStyle.Colors[ImGuiNeoSequencerCol_ZoomBarSliderEndsHovered] = ImVec4{0.93f, 0.0f, 0.0f, 0.93f};

	neoStyle.Colors[ImGuiNeoSequencerCol_SelectionBorder] = style.Colors[ImGuiCol_Border];
	neoStyle.Colors[ImGuiNeoSequencerCol_Selection] = style.Colors[ImGuiCol_FrameBgActive];
}

void StyleImGuizmo() {
	ImGuizmo::Style &guizmoStyle = ImGuizmo::GetStyle();
	const ImGuiStyle &style = ImGui::GetStyle();

	// reset everything
	guizmoStyle = ImGuizmo::Style();

	// guizmoStyle.Colors[ImGuizmo::DIRECTION_X] = ;
	// guizmoStyle.Colors[ImGuizmo::DIRECTION_Y] = ;
	// guizmoStyle.Colors[ImGuizmo::DIRECTION_Z] = ;
	// guizmoStyle.Colors[ImGuizmo::PLANE_X] = ;
	// guizmoStyle.Colors[ImGuizmo::PLANE_Y] = ;
	// guizmoStyle.Colors[ImGuizmo::PLANE_Z] = ;
	// guizmoStyle.Colors[ImGuizmo::SELECTION] = ;
	// guizmoStyle.Colors[ImGuizmo::INACTIVE] = ;
	// guizmoStyle.Colors[ImGuizmo::TRANSLATION_LINE] = ;
	// guizmoStyle.Colors[ImGuizmo::SCALE_LINE] = ;
	// guizmoStyle.Colors[ImGuizmo::ROTATION_USING_BORDER] = ;
	guizmoStyle.Colors[ImGuizmo::ROTATION_USING_FILL] = guizmoStyle.Colors[ImGuizmo::ROTATION_USING_BORDER];
	// guizmoStyle.Colors[ImGuizmo::HATCHED_AXIS_LINES] = ;
	guizmoStyle.Colors[ImGuizmo::TEXT] = style.Colors[ImGuiCol_Text];
	guizmoStyle.Colors[ImGuizmo::TEXT_SHADOW] = style.Colors[ImGuiCol_Text];
}

} // namespace ImGui
