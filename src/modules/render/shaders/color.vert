layout(std140) uniform u_uniformblock {
	mat4 u_viewprojection;
	mat4 u_model;
};

layout(location = 1) $in vec4 a_pos;
layout(location = 2) $in vec4 a_color;

$out vec4 v_color;

void main()
{
	v_color = a_color;
	gl_Position = u_viewprojection * u_model * a_pos;
}
