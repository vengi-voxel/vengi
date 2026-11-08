/**
 * @brief Shader to fill the bound shadowmap with the depth values
 */

layout(location = 0) in vec3 a_pos;

#ifdef USEDRAWPARAMETERS
#include "_drawinstance.glsl"
#endif

layout(std140, binding = 0) uniform u_block {
	mat4 u_lightviewprojection;
	mat4 u_model;
	int u_usedrawinstances;
};

void main() {
	mat4 model = u_model;
#ifdef USEDRAWPARAMETERS
	if (u_usedrawinstances > 0) model = u_drawinstances[gl_InstanceID].model;
#endif
	vec4 worldpos = model * vec4(a_pos, 1.0f);
	gl_Position = u_lightviewprojection * worldpos;
}
