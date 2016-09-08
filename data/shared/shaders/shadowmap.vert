$in uvec3 a_pos;

uniform mat4 u_light;
uniform mat4 u_light_inverse;
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_view_inverse;
uniform mat4 u_projection;
uniform mat4 u_projection_inverse;
uniform vec3 u_campos;
$out vec2 o_projZW;

void main()
{
	vec4 pos = u_light * u_model * vec4(a_pos + u_campos, 1.0f);
	gl_Position = pos;
	o_projZW = gl_Position.zw;
}
