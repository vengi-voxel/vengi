/**
 * @file
 */
#include "BrushHandleUI.h"
#include "ui/IMGUIEx.h"
#include "ui/Style.h"
#include "ui/dearimgui/ImGuizmo.h"
#include "video/Camera.h"
#include "voxedit-util/modifier/Modifier.h"
#include <glm/gtc/type_ptr.hpp>

namespace voxedit {
static bool drawHandle(Modifier &modifier, const BrushGizmoState &state, const video::Camera &camera, float headerSize,
					   const glm::vec2 &dpiScale, int viewportId) {
	// Map BrushGizmoOperation to ImGuizmo::OPERATION
	uint32_t imguizmoOp = 0;
	if (state.operations & (BrushGizmo_Translate | BrushGizmo_BezierControl)) {
		imguizmoOp |= ImGuizmo::TRANSLATE;
	}
	if (state.operations & BrushGizmo_TranslateX) {
		imguizmoOp |= ImGuizmo::TRANSLATE_X;
	}
	if (state.operations & BrushGizmo_TranslateY) {
		imguizmoOp |= ImGuizmo::TRANSLATE_Y;
	}
	if (state.operations & BrushGizmo_TranslateZ) {
		imguizmoOp |= ImGuizmo::TRANSLATE_Z;
	}
	if (state.operations & BrushGizmo_Rotate) {
		imguizmoOp |= ImGuizmo::ROTATE;
	}
	if (state.operations & BrushGizmo_Scale) {
		imguizmoOp |= ImGuizmo::SCALE;
	}
	if (state.operations & BrushGizmo_Bounds) {
		imguizmoOp |= ImGuizmo::BOUNDS;
	}
	if (state.operations & BrushGizmo_Line) {
		if (state.numPositions >= 2) {
			ImVec2 windowPos = ImGui::GetWindowPos();
			windowPos.y += headerSize;
			const glm::vec2 scale = dpiScale;
			ImDrawList *drawList = ImGui::GetWindowDrawList();
			const ImU32 lineColor = ImGui::GetColorU32(ImVec4(style::color(style::ColorBrushGizmoLine)));
			const float lineThickness = 2.0f;
			for (int i = 0; i < state.numPositions - 1; ++i) {
				const glm::ivec2 screenA = camera.worldToScreen(state.positions[i]);
				const glm::ivec2 screenB = camera.worldToScreen(state.positions[i + 1]);
				const ImVec2 a(windowPos.x + (float)screenA.x / scale.x, windowPos.y + (float)screenA.y / scale.y);
				const ImVec2 b(windowPos.x + (float)screenB.x / scale.x, windowPos.y + (float)screenB.y / scale.y);
				drawList->AddLine(a, b, lineColor, lineThickness);
			}
		}
	}

	if (state.operations & BrushGizmo_ScreenPolygon) {
		if (state.screenPolygon != nullptr && state.screenPolygon->size() >= 2) {
			const auto &points = *state.screenPolygon;
			ImDrawList *drawList = ImGui::GetWindowDrawList();
			const ImVec2 windowPos = ImGui::GetWindowPos();
			const float offsetY = headerSize;
			const glm::vec2 scale = dpiScale;
			const int n = (int)points.size();
			ImVec2 *imPoints = (ImVec2 *)alloca(sizeof(ImVec2) * n);
			for (int i = 0; i < n; ++i) {
				imPoints[i] =
					ImVec2(windowPos.x + points[i].x / scale.x, windowPos.y + offsetY + points[i].y / scale.y);
			}
			drawList->AddConcavePolyFilled(imPoints, n,
										   ImGui::GetColorU32(ImVec4(style::color(style::ColorBrushGizmoPolygon))));
			const glm::vec4 outlineColor = glm::vec4(glm::vec3(style::color(style::ColorBrushGizmoPolygon)), 0.8f);
			drawList->AddPolyline(imPoints, n, ImGui::GetColorU32(ImVec4(outlineColor)), 1.5f, ImDrawFlags_Closed);
		}
	}

	// A handle may appear under the mouse on the first frame of a box drag.
	// The action button already owns that press; do not let ImGuizmo capture it.
	if (imguizmoOp == 0 || modifier.isBrushActionActive()) {
		return false;
	}

	const ImGuizmo::MODE mode = state.localMode ? ImGuizmo::MODE::LOCAL : ImGuizmo::MODE::WORLD;
	const float *snapPtr =
		(state.snap[0] > 0.0f || state.snap[1] > 0.0f || state.snap[2] > 0.0f) ? state.snap : nullptr;
	const float *boundsPtr = state.hasBounds ? state.bounds : nullptr;
	static const float boundsSnap[] = {1.0f, 1.0f, 1.0f};

	glm::mat4 matrix = state.matrix;
	glm::mat4 deltaMatrix(1.0f);
	float *mPtr = glm::value_ptr(matrix);
	float *dMatPtr = glm::value_ptr(deltaMatrix);
	const float *vMatPtr = glm::value_ptr(camera.viewMatrix());
	const float *pMatPtr = glm::value_ptr(camera.projectionMatrix());

	const bool manipulated = ImGuizmo::Manipulate(vMatPtr, pMatPtr, (ImGuizmo::OPERATION)imguizmoOp, mode, mPtr,
												  dMatPtr, snapPtr, boundsPtr, boundsSnap);

	// Track whether the brush gizmo is being interacted with so the action
	// button (ModifierButton) does not commit intermediate transforms
	BrushHandleEvent event;
	event.operationId = state.operationId;
	event.handleId = state.id;
	event.viewportId = viewportId;
	if (ImGuizmo::IsUsing()) {
		event.phase = BrushHandlePhase::Begin;
		if (!modifier.editBrushHandle(event)) {
			return false;
		}
	}

	if (!manipulated) {
		if (!ImGuizmo::IsUsing()) {
			event.phase = BrushHandlePhase::End;
			modifier.editBrushHandle(event);
		}
		return false;
	}

	event.phase = BrushHandlePhase::Change;
	event.matrix = matrix;
	event.delta = deltaMatrix;
	event.operation = state.operations;
	const bool changed = modifier.editBrushHandle(event);
	if (!ImGuizmo::IsUsing()) {
		event.phase = BrushHandlePhase::End;
		modifier.editBrushHandle(event);
	}
	return changed;
}

bool drawBrushHandles(Modifier &modifier, const video::Camera &camera, float headerSize, const glm::vec2 &dpiScale,
					  int viewportId) {
	core::DynamicArray<BrushGizmoState> handles;
	modifier.brushHandles(handles);
	bool changed = false;
	for (const auto &handle : handles) {
		ImGuizmo::PushID((int)handle.id);
		changed |= drawHandle(modifier, handle, camera, headerSize, dpiScale, viewportId);
		ImGuizmo::PopID();
	}
	return changed;
}
} // namespace voxedit
