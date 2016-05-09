uniform mat4 u_view;
uniform mat4 u_projection;

in vec4 a_pos;
in vec3 a_color;

out vec3 v_color;

void main()
{
	v_color = a_color;
	gl_Position = u_projection * u_view * a_pos;
}