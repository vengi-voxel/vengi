$in vec3 a_pos;
#ifdef INSTANCED
// instanced rendering
$in vec3 a_offset;
#endif

uniform mat4 u_light;
$out vec2 o_projZW;

void main()
{
#ifdef INSTANCED
	vec4 pos = vec4(a_offset, 0.0) + u_light * vec4(a_pos, 1.0f);
#else
	vec4 pos = u_light * vec4(a_pos, 1.0f);
#endif
	gl_Position = pos;
	o_projZW = pos.zw;
}
