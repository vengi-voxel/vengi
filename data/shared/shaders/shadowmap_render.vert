#include "_shadowmap.vert"

uniform vec3 u_campos;

$in vec3 a_pos;
$in vec2 a_texcoord;
$out vec2 v_texcoord;

void main(void) {
	v_texcoord = a_texcoord;
	gl_Position = vec4(a_pos, 1.0);
	v_lightspacepos = u_light * (vec4(u_campos, 1.0) + gl_Position);
}
