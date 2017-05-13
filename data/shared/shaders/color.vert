uniform mat4 u_viewprojection;
uniform mat4 u_model;

$in vec4 a_pos;
$in vec3 a_color;

$out vec3 v_color;

void main()
{
	v_color = a_color;
	gl_Position = u_viewprojection * u_model * a_pos;
}