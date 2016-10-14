/**
 * @brief Shader to fill the bound shadowmap with the depth values
 */

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
	vec4 worldpos = u_model * vec4(a_pos, 1.0f);
#ifdef INSTANCED
	worldpos = vec4(a_offset, 0.0) + worldpos;
#endif
	vec4 pos = u_light * worldpos;
	gl_Position = pos;
	o_projZW = pos.zw;
}
