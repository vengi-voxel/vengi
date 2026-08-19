
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
};

// Per-draw instance data for multi-draw-indirect. Binding 3 is free
// (0=vert UBO, 1=frag UBO, 2=shadow sampler). Extensions are injected by Shader::getSource.
// Prefer gl_DrawIDARB so MDI works below GLSL 460 when ARB_shader_draw_parameters is present.
#ifdef USEDRAWPARAMETERS
struct DrawInstance {
	mat4 model;
	int gray;
	int locked;
	float opacity;
	int pad;
};
layout(std430, binding = 3) readonly buffer DrawInstanceBuffer {
	DrawInstance u_drawinstances[];
};
#if __VERSION__ >= 460
#define VENGIDRAWID gl_DrawID
#else
#define VENGIDRAWID gl_DrawIDARB
#endif
mat4 getModelMatrix() { return u_drawinstances[VENGIDRAWID].model; }
int getGrayFlag() { return u_drawinstances[VENGIDRAWID].gray; }
int getLockedFlag() { return u_drawinstances[VENGIDRAWID].locked; }
float getOpacity() { return u_drawinstances[VENGIDRAWID].opacity; }
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
