layout(location = 1) $in vec3 a_pos;

uniform mat4 u_projection;
uniform mat4 u_view;

$out vec3 v_uv;

void main()
{
	v_uv = a_pos;
	vec4 pos = u_projection * u_view * vec4(a_pos, 1.0);
	gl_Position = pos.xyww;
}
