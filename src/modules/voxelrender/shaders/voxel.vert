// attributes from the VAOs
$in vec3 a_pos;
$in uvec2 a_info;

#if INSTANCED > 0
$constant MaxInstances INSTANCED
uniform mat4 u_model[INSTANCED];
uniform vec3 u_pivot[INSTANCED];
#else
uniform mat4 u_model;
uniform vec3 u_pivot;
#endif
uniform mat4 u_viewprojection;
uniform int u_gray;

$out vec4 v_pos;
$out vec4 v_color;
$out vec4 v_glow;
$out float v_ambientocclusion;
flat $out uint v_flags;

#include "_voxelflags.glsl"
#include "_material.vert"
#include "_shadowmap.vert"
#include "_ambientocclusion.vert"

void main(void) {
	uint a_ao = (a_info[0] & 3u);
	uint a_flags = ((a_info[0] & ~3u) >> 2u);
	uint a_colorindex = a_info[1];
#if INSTANCED > 0
	v_pos = u_model[gl_InstanceID] * vec4(a_pos - u_pivot[gl_InstanceID], 1.0);
#else
	v_pos = u_model * vec4(a_pos - u_pivot, 1.0);
#endif

	int materialColorIndex = int(a_colorindex);
	vec4 materialColor = u_materialcolor[materialColorIndex];
	vec4 glowColor = u_glowcolor[materialColorIndex];
	v_flags = 0u;
#if r_renderoutline == 0
	if ((a_flags & FLAGOUTLINE) != 0u)
#endif
		v_flags |= FLAGOUTLINE;
	if ((a_flags & FLAGBLOOM) != 0u)
		v_flags |= FLAGBLOOM;

	if (u_gray != 0) {
		float gray = (0.21 * materialColor.r + 0.72 * materialColor.g + 0.07 * materialColor.b) / 3.0;
		v_color = vec4(gray, gray, gray, materialColor.a);
	} else {
		v_color = materialColor;
	}
	v_glow = glowColor;
	v_ambientocclusion = aovalues[a_ao];

#if cl_shadowmap == 1
	v_lightspacepos = v_pos.xyz;
	v_viewz = (u_viewprojection * vec4(v_lightspacepos, 1.0)).w;
#endif // cl_shadowmap

	gl_Position = u_viewprojection * v_pos;
}
