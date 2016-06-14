// renders a quad fullscreen from -1.0, -1.0 - 1.0, 1.0

$in vec3 a_pos;
#ifdef TEXTURED
$in vec2 a_texcoord;
$out vec2 v_texcoord;
#endif

void main(void) {
#ifdef TEXTURED
	v_texcoord = a_texcoord;
#endif
	gl_Position = vec4(a_pos, 1.0);
}
