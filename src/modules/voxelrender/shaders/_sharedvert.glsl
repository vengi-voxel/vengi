
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
	int u_vertrenderoutline;
	int u_shownormals;
	float u_opacity;
	// 1 = read per-draw model/flags from the draw-instance SSBO (multi-draw batches).
	// 0 = use u_model/u_gray/... (unique meshes; avoids a useless SSBO update per draw).
	int u_usedrawinstances;
};

#ifdef VULKAN
// Per-draw model/flags. Lets the Vulkan backend keep the palette UBO stable
// across unique-mesh draws and only push the per-draw fields (see
// setUniformBufferPushOverlay / DrawInstanceData layout).
layout(push_constant) uniform DrawPush {
	mat4 model;
	int gray;
	int locked;
	float opacity;
	int _pad;
} drawPush;
#endif

#ifdef USEDRAWPARAMETERS
#include "_drawinstance.glsl"
mat4 getModelMatrix() {
	if (u_usedrawinstances != 0) {
		return u_drawinstances[VENGIDRAWID].model;
	}
#ifdef VULKAN
	return drawPush.model;
#else
	return u_model;
#endif
}
int getGrayFlag() {
	if (u_usedrawinstances != 0) {
		return u_drawinstances[VENGIDRAWID].gray;
	}
#ifdef VULKAN
	return drawPush.gray;
#else
	return u_gray;
#endif
}
int getLockedFlag() {
	if (u_usedrawinstances != 0) {
		return u_drawinstances[VENGIDRAWID].locked;
	}
#ifdef VULKAN
	return drawPush.locked;
#else
	return u_locked;
#endif
}
float getOpacity() {
	if (u_usedrawinstances != 0) {
		return u_drawinstances[VENGIDRAWID].opacity;
	}
#ifdef VULKAN
	return drawPush.opacity;
#else
	return u_opacity;
#endif
}
#else
#ifdef VULKAN
mat4 getModelMatrix() { return drawPush.model; }
int getGrayFlag() { return drawPush.gray; }
int getLockedFlag() { return drawPush.locked; }
float getOpacity() { return drawPush.opacity; }
#else
mat4 getModelMatrix() { return u_model; }
int getGrayFlag() { return u_gray; }
int getLockedFlag() { return u_locked; }
float getOpacity() { return u_opacity; }
#endif
#endif

$out vec3 v_pos;
$out vec3 v_normal;
$out vec4 v_color;
$out vec4 v_glow;
flat $out uint v_flags;

$out vec3 v_lightspacepos;
flat $out vec3 v_nm0;
flat $out vec3 v_nm1;
flat $out vec3 v_nm2;

const float aovalues[] = float[](0.15, 0.6, 0.8, 1.0);

void writeNormalMatrix() {
	mat4 m = getModelMatrix();
	vec3 c0 = vec3(m[0][0], m[0][1], m[0][2]);
	vec3 c1 = vec3(m[1][0], m[1][1], m[1][2]);
	vec3 c2 = vec3(m[2][0], m[2][1], m[2][2]);
	float det = dot(c0, cross(c1, c2));
	float invDet = (abs(det) > 1.0e-8) ? (1.0 / det) : 1.0;
	v_nm0 = cross(c1, c2) * invDet;
	v_nm1 = cross(c2, c0) * invDet;
	v_nm2 = cross(c0, c1) * invDet;
}
