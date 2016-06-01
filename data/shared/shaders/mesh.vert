// attributes from the VAOs
$in vec3 a_pos;
$in vec3 a_norm;
$in vec2 a_texcoords;

uniform mat4 u_projection;
uniform mat4 u_model;
uniform mat4 u_view;
uniform float u_fogrange;
uniform float u_viewdistance;
uniform vec3 u_lightpos;

$out vec3 v_pos;
$out vec3 v_norm;
$out vec2 v_texcoords;
$out vec3 v_lightpos;
$out float v_fogrange;
$out float v_viewdistance;

void main(void) {
	mat4 modelview = u_view * u_model;
	vec4 pos4 = modelview * vec4(a_pos, 1.0);
	v_pos = vec3(pos4) / pos4.w;
	v_norm = a_norm;
	v_texcoords = a_texcoords;
	v_fogrange = u_fogrange;
	v_viewdistance = u_viewdistance;
	v_lightpos = u_lightpos;
	gl_Position = u_projection * modelview * vec4(a_pos, 1.0);
}
