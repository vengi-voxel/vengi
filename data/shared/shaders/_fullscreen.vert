// renders a quad fullscreen from -1.0, -1.0 - 1.0, 1.0

$in vec3 a_pos;

void main(void) {
	gl_Position = vec4(a_pos, 1.0);
}
