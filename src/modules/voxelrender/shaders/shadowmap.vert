/**
 * @brief Shader to fill the bound shadowmap with the depth values
 */

layout(location = 0) in vec3 a_pos;

#if defined(VULKAN) && !defined(USEDRAWPARAMETERS)
#define USEDRAWPARAMETERS 1
#endif

#ifdef USEDRAWPARAMETERS
#include "_drawinstance.glsl"
#ifdef VULKAN
#define VENGIINSTANCEINDEX gl_InstanceIndex
#elif __VERSION__ >= 460
#define VENGIINSTANCEINDEX (gl_BaseInstance + gl_InstanceID)
#else
#define VENGIINSTANCEINDEX (gl_BaseInstanceARB + gl_InstanceID)
#endif
#endif

layout(std140, binding = 0) uniform u_block {
	mat4 u_lightviewprojection;
	mat4 u_model;
	int u_usedrawinstances;
};

void main() {
	mat4 model = u_model;
#ifdef USEDRAWPARAMETERS
	if (u_usedrawinstances > 0) model = u_drawinstances[VENGIINSTANCEINDEX].model;
#endif
	vec4 worldpos = model * vec4(a_pos, 1.0f);
	gl_Position = u_lightviewprojection * worldpos;
}
