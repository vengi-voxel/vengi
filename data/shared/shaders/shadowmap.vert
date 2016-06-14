$in vec3 a_pos;
#ifdef INSTANCED
// instanced rendering
$in vec3 a_offset;
#endif

uniform mat4 u_light;
uniform mat4 u_model;

void main()
{
#ifdef INSTANCED
	gl_Position = vec4(a_offset, 0.0) + u_light * u_model * vec4(a_pos, 1.0f);
#else
	gl_Position = u_light * u_model * vec4(a_pos, 1.0f);
#endif
}
