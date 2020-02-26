// attributes from the VAOs
$in vec2 a_pos;

uniform mat4 u_model;
uniform mat4 u_viewprojection;

$out vec3 v_pos;

#include "_fog.vert"
#include "_shadowmap.vert"

const float tiling = 6.0;

void main(void) {
	vec4 pos = u_model * vec4(a_pos.x, 0.0, a_pos.y, 1.0);
	v_pos = pos.xyz;

#if cl_shadowmap == 1
	v_lightspacepos = v_pos;
	v_viewz = (u_viewprojection * vec4(v_lightspacepos, 1.0)).w;
#endif // cl_shadowmap

	gl_Position = u_viewprojection * vec4(v_pos, 1.0);
}
