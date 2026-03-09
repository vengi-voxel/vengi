/**
 * @file
 * @brief Types for per-brush viewport gizmos
 * @ingroup Brushes
 */

#pragma once

#include <glm/mat4x4.hpp>

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
	BrushGizmo_Shear = (1u << 4), // Custom - not in vanilla ImGuizmo
	BrushGizmo_TranslateX = (1u << 5),
	BrushGizmo_TranslateY = (1u << 6),
	BrushGizmo_TranslateZ = (1u << 7),
};

/**
 * @brief Describes a gizmo that a brush wants to contribute to the viewport
 *
 * The Viewport queries the active brush each frame via @c Brush::wantBrushGizmo()
 * and, when true, reads this state via @c Brush::brushGizmoState(). The Viewport
 * then uses ImGuizmo to render and interact with the gizmo, feeding results back
 * through @c Brush::applyBrushGizmo().
 */
struct BrushGizmoState {
	/** The gizmo transform matrix (position/rotation/scale of the gizmo) */
	glm::mat4 matrix{1.0f};

	/** Which operations the gizmo supports (bitmask of BrushGizmoOperation) */
	uint32_t operations = BrushGizmo_None;

	/** Local-space bounds for BOUNDS operation (minX, minY, minZ, maxX, maxY, maxZ) */
	float bounds[6] = {};

	/** Whether the bounds array is valid and BrushGizmo_Bounds should use them */
	bool hasBounds = false;

	/** Snap increment (0 = no snapping, typically grid resolution) */
	float snap = 1.0f;

	/** Whether to use LOCAL or WORLD mode for the gizmo */
	bool localMode = true;
};

} // namespace voxedit
