uniform mat4 u_view;
uniform mat4 u_model;
uniform mat4 u_projection;
$in vec3 a_pos;
$in vec2 a_texcoord;
$out vec2 v_texcoord;

void main(void) {
	v_texcoord = a_texcoord;
	gl_Position = u_projection * u_view * u_model * vec4(a_pos, 1.0);
}
