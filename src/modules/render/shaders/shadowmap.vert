/**
 * @brief Shader to fill the bound shadowmap with the depth values
 */

$in vec3 a_pos;

uniform mat4 u_lightviewprojection;
#if INSTANCED > 0
$constant MaxInstances INSTANCED
uniform mat4 u_model[INSTANCED];
uniform vec3 u_pivot[INSTANCED];
#else
uniform mat4 u_model;
uniform vec3 u_pivot;
#endif

void main()
{
#if INSTANCED > 0
	vec4 worldpos = u_model[gl_InstanceID] * vec4(a_pos, 1.0f) - vec4(u_pivot[gl_InstanceID], 0.0);
#else
	vec4 worldpos = u_model * vec4(a_pos, 1.0f) - vec4(u_pivot, 0.0);
#endif
	gl_Position = u_lightviewprojection * worldpos;
}
