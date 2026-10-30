/**
 * @file
 * @brief Types for per-brush viewport gizmos
 * @ingroup Brushes
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace voxedit {

/**
 * @brief Operations a brush gizmo can support
 *
 * These map to ImGuizmo::OPERATION flags where possible.
 * BrushGizmo_Shear is a custom extension not present in vanilla ImGuizmo.
 */
enum BrushGizmoOperation : uint32_t {
	BrushGizmo_None = 0,
	BrushGizmo_Translate = (1u << 0),
	BrushGizmo_Rotate = (1u << 1),
	BrushGizmo_Scale = (1u << 2),
	BrushGizmo_Bounds = (1u << 3),
	BrushGizmo_TranslateX = (1u << 4),
	BrushGizmo_TranslateY = (1u << 5),
	BrushGizmo_TranslateZ = (1u << 6),
	BrushGizmo_BezierControl = (1u << 7),
	BrushGizmo_Line = (1u << 8),
	/** Screen-space polygon overlay (e.g. lasso selection) - does not use ImGuizmo */
	BrushGizmo_ScreenPolygon = (1u << 9),
};

/**
 * @brief Describes a gizmo that a brush wants to contribute to the viewport
 *
 * The active brush appends these descriptions through @c Brush::handles().
 * BrushHandleUI renders handles and overlays, and forwards captured interactions
 * through Modifier to @c Brush::editHandle().
 */
struct BrushGizmoState {
	uint32_t id = 0;
	uint64_t operationId = 0;
	/** The gizmo transform matrix (position/rotation/scale of the gizmo) */
	glm::mat4 matrix{1.0f};

	/** Which operations the gizmo supports (bitmask of BrushGizmoOperation) */
	uint32_t operations = BrushGizmo_None;

	/** Local-space bounds for BOUNDS operation (minX, minY, minZ, maxX, maxY, maxZ) */
	float bounds[6] = {};

	/** Whether the bounds array is valid and BrushGizmo_Bounds should use them */
	bool hasBounds = false;

	/** Snap increment per axis (0 = no snapping, typically grid resolution) */
	float snap[3] = {1.0f, 1.0f, 1.0f};

	/** Whether to use LOCAL or WORLD mode for the gizmo */
	bool localMode = true;

	/** Line positions for BrushGizmo_Line - drawn as a polyline from positions[0..numPositions-1] */
	glm::vec3 positions[8] = {};
	/** Number of valid entries in positions[] */
	int numPositions = 0;

	/** Screen-space polygon points for BrushGizmo_ScreenPolygon (e.g. lasso) */
	const core::DynamicArray<glm::vec2> *screenPolygon = nullptr;
};

enum class BrushHandlePhase { Begin, Change, End };
struct BrushHandleEvent {
	uint64_t operationId = 0;
	uint32_t handleId = 0;
	int viewportId = -1;
	BrushHandlePhase phase = BrushHandlePhase::Change;
	glm::mat4 matrix{1.0f};
	glm::mat4 delta{1.0f};
	uint32_t operation = BrushGizmo_None;
};

} // namespace voxedit
