layout(location = 0) $in vec3 a_pos;

layout(std140, binding = 0) uniform u_vert {
	mat4 u_viewprojection;
	mat4 u_model;
};

$out vec3 v_pos;

void main() {
	v_pos = a_pos;
	gl_Position = u_viewprojection * (u_model * vec4(a_pos, 1.0));
}
