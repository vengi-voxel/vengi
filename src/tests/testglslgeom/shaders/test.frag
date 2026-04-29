$in vec3 v_color;
layout(location = 0) $out vec4 o_color;

void main() {
	o_color = vec4(v_color, 1.0);
}
