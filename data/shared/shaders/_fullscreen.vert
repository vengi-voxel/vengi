// renders a quad fullscreen from 0.0, 0.0 - 1.0, 1.0

$in vec3 a_pos;

void main(void) {
	gl_Position = mat4() * vec4(a_pos, 1.0);
}
