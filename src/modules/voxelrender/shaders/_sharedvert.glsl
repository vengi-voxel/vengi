
// PaletteMaxColors
#define MATERIALCOLORS 256
// NormalPaletteMaxNormals + NORMAL_OFFSET
#define NORMALS 256
layout(std140) uniform u_vert {
	vec4 u_materialcolor[MATERIALCOLORS];
	vec4 u_normals[NORMALS];
	vec4 u_glowcolor[MATERIALCOLORS];
	mat4 u_viewprojection;
	mat4 u_model;
	int u_gray;
	int u_padding1;
	int u_padding2;
	int u_padding3;
};

$out vec3 v_pos;
$out vec3 v_normal;
$out vec4 v_color;
$out vec4 v_glow;
flat $out uint v_flags;

#if cl_shadowmap == 1
$out vec3 v_lightspacepos;
$out float v_viewz;
#endif

const float aovalues[] = float[](0.15, 0.6, 0.8, 1.0);
