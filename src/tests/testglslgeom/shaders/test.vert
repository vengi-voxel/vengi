layout(location = 0) $in vec4 a_pos;
layout(location = 1) $in vec3 a_color;
layout(std140, binding = 0) uniform u_vert {
	mat4 u_view;
};
$out vec3 g_color;

void main() {
	gl_Position = u_view * a_pos;
	g_color = a_color;
}
