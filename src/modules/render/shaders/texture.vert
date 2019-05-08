// attributes from the VAOs
$in vec2 a_pos;
$in vec2 a_texcoord;
$in vec4 a_color;

uniform mat4 u_viewprojection;
uniform mat4 u_model;

$out vec2 v_texcoord;
$out vec4 v_color;

void main(void) {
	v_color = a_color;
	v_texcoord = a_texcoord;
	gl_Position = u_viewprojection * u_model * vec4(a_pos.x, a_pos.y, 0.0, 1.0);
}
