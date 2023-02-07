/**
 * @brief Shader to fill the bound shadowmap with the depth values
 */

$in vec3 a_pos;

uniform mat4 u_lightviewprojection;
uniform mat4 u_model;
uniform vec3 u_pivot;

void main()
{
	vec4 worldpos = u_model * vec4(a_pos - u_pivot, 1.0f);
	gl_Position = u_lightviewprojection * worldpos;
}
