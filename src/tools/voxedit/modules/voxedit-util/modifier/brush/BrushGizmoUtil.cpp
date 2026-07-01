/**
 * @file
 */

#include "BrushGizmoUtil.h"
#include "core/Log.h"

#include <SDL3/SDL_stdinc.h>

namespace voxedit {

uint32_t mapBrushGizmoOperation(const char *name) {
	if (SDL_strcmp(name, "translate") == 0) {
		return BrushGizmo_Translate;
	}
	if (SDL_strcmp(name, "translatex") == 0) {
		return BrushGizmo_TranslateX;
	}
	if (SDL_strcmp(name, "translatey") == 0) {
		return BrushGizmo_TranslateY;
	}
	if (SDL_strcmp(name, "translatez") == 0) {
		return BrushGizmo_TranslateZ;
	}
	if (SDL_strcmp(name, "rotate") == 0) {
		return BrushGizmo_Rotate;
	}
	if (SDL_strcmp(name, "scale") == 0) {
		return BrushGizmo_Scale;
	}
	if (SDL_strcmp(name, "bounds") == 0) {
		return BrushGizmo_Bounds;
	}
	if (SDL_strcmp(name, "line") == 0) {
		return BrushGizmo_Line;
	}
	Log::warn("Unknown gizmo operation: %s", name);
	return BrushGizmo_None;
}

} // namespace voxedit
