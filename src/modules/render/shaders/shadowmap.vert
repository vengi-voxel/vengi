/**
 * @brief Shader to fill the bound shadowmap with the depth values
 */

$in vec3 a_pos;
#ifdef INSTANCED
// instanced rendering
$in vec3 a_offset;
#endif

uniform mat4 u_lightviewprojection;
uniform mat4 u_model;

void main()
{
	vec4 worldpos = u_model * vec4(a_pos, 1.0f);
#ifdef INSTANCED
	worldpos = vec4(a_offset, 0.0) + worldpos;
#endif
	gl_Position = u_lightviewprojection * worldpos;
}
