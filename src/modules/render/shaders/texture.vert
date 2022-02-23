// attributes from the VAOs
$in vec2 a_pos;
$in vec2 a_texcoord;

$out vec2 v_texcoord;

void main(void) {
	v_texcoord = a_texcoord;
	gl_Position = vec4(a_pos.x, a_pos.y, 0.0, 1.0);
}
