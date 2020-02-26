// attributes from the VAOs
$in vec2 a_pos;

uniform vec3 u_camerapos;
uniform mat4 u_model;
uniform mat4 u_viewprojection;

$out vec3 v_camdist;
$out vec3 v_pos;
$out vec4 v_clipspace;
$out vec2 v_uv;

#include "_fog.vert"
#include "_shadowmap.vert"

const float tiling = 4.0;

void main(void) {
	vec4 pos = u_model * vec4(a_pos.x, 0.0, a_pos.y, 1.0);
	v_pos = pos.xyz;

#if cl_shadowmap == 1
	v_lightspacepos = v_pos;
	v_viewz = (u_viewprojection * vec4(v_lightspacepos, 1.0)).w;
#endif // cl_shadowmap
	v_clipspace = u_viewprojection * pos;
	// convert the water plane positions into texcoords for the distortion texture
	v_uv = vec2(a_pos.x / 2.0 + 0.5, a_pos.y / 2.0 + 0.5) * tiling;
	v_camdist = u_camerapos - v_pos;

	gl_Position = v_clipspace;
}
