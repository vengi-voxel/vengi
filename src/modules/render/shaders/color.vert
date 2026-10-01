layout(std140, binding = 0) uniform u_uniformblock {
	mat4 u_viewprojection;
	mat4 u_model;
	vec4 u_lightpos;
	// 4th component is the ambient strength - and if this is 0, no
	// lighting calculations are done
	vec4 u_lightcolor;
};

layout(location = 1) $in vec4 a_pos;
layout(location = 2) $in vec4 a_color;
layout(location = 3) $in vec3 a_normal;

$out vec4 v_color;
$out vec3 v_normal;
$out vec4 v_fragpos;
$out vec4 v_lightpos;
$out vec4 v_lightcolor;

void main()
{
	vec4 fragPos = u_model * a_pos;

	v_color = a_color;
	v_normal = a_normal;
	v_fragpos = fragPos;
	v_lightpos = u_lightpos;
	v_lightcolor = u_lightcolor;

	gl_Position = u_viewprojection * fragPos;
}
