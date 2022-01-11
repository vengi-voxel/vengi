/**
 * @brief Shader to fill the bound shadowmap with the depth values
 */

$in vec3 a_pos;

uniform mat4 u_lightviewprojection;
#if INSTANCED > 0
uniform mat4 u_model[INSTANCED];
#else
uniform mat4 u_model;
#endif

void main()
{
#if INSTANCED > 0
	vec4 worldpos = u_model[gl_InstanceID] * vec4(a_pos, 1.0f);
#else
	vec4 worldpos = u_model * vec4(a_pos, 1.0f);
#endif
	gl_Position = u_lightviewprojection * worldpos;
}
