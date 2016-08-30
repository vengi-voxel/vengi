$in uvec3 a_pos;
#ifdef INSTANCED
// instanced rendering
$in vec3 a_offset;
#endif

uniform mat4 u_light;
uniform mat4 u_model;
$out vec2 o_projZW;

void main()
{
#ifdef INSTANCED
	vec4 pos = vec4(a_offset, 0.0) + u_light * u_model * vec4(a_pos, 1.0f);
#else
	vec4 pos = u_light * u_model * vec4(a_pos, 1.0f);
#endif
	gl_Position = pos;
	o_projZW = vec2(1.0 - pos.z, pos.w);
}
