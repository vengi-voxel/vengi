
// PaletteMaxColors
#define MATERIALCOLORS 256
// NormalPaletteMaxNormals + NORMAL_PALETTE_OFFSET
#define NORMALS 256
layout(std140, binding = 0) uniform u_vert {
	vec4 u_materialcolor[MATERIALCOLORS];
	vec4 u_normals[NORMALS];
	vec4 u_glowcolor[MATERIALCOLORS];
	mat4 u_viewprojection;
	mat4 u_model;
	int u_gray;
	int u_locked;
	int u_vert_renderoutline;
	int u_shownormals;
	float u_opacity;
	// 1 = read per-draw model/flags from the draw-instance SSBO (multi-draw batches).
	// 0 = use u_model/u_gray/... (unique meshes; avoids a useless SSBO update per draw).
	int u_useDrawInstances;
};

#ifdef USEDRAWPARAMETERS
#include "_drawinstance.glsl"
mat4 getModelMatrix() {
	if (u_useDrawInstances != 0) {
		return u_drawinstances[VENGIDRAWID].model;
	}
	return u_model;
}
int getGrayFlag() {
	if (u_useDrawInstances != 0) {
		return u_drawinstances[VENGIDRAWID].gray;
	}
	return u_gray;
}
int getLockedFlag() {
	if (u_useDrawInstances != 0) {
		return u_drawinstances[VENGIDRAWID].locked;
	}
	return u_locked;
}
float getOpacity() {
	if (u_useDrawInstances != 0) {
		return u_drawinstances[VENGIDRAWID].opacity;
	}
	return u_opacity;
}
#else
mat4 getModelMatrix() { return u_model; }
int getGrayFlag() { return u_gray; }
int getLockedFlag() { return u_locked; }
float getOpacity() { return u_opacity; }
#endif

$out vec3 v_pos;
$out vec3 v_normal;
$out vec4 v_color;
$out vec4 v_glow;
flat $out uint v_flags;

$out vec3 v_lightspacepos;
$out float v_viewz;

const float aovalues[] = float[](0.15, 0.6, 0.8, 1.0);
