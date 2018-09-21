uniform mat4 u_viewprojection;
uniform mat4 u_model;

$in vec3 a_pos;
$in vec3 a_norm;

$out vec3 v_norm;

void main()
{
	gl_Position = u_viewprojection * u_model * vec4(a_pos, 1.0);
	v_norm = a_norm;
}