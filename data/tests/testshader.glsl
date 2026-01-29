$in vec2 a_pos;
$in vec2 a_texcoord;
$in vec4 a_color;

#define FLAGBLOOM 2u
$constant FlagBloom FLAGBLOOM

#define MATERIALCOLORS 256
layout(std140) uniform u_materialblock {
	vec4 u_materialcolor[MATERIALCOLORS];
	vec4 u_glowcolor[MATERIALCOLORS];
	vec4 u_somevec4;
	lowp vec4 u_somevec4lowp;
	vec3 foobar1;
	float foobar2;
	vec4 foobar3;
	vec2 foobar4;
};

// Shader Storage Buffer Object with std430 layout
layout(std430, binding = 0) buffer ParticleBuffer {
	vec4 positions[64];
	vec4 velocities[64];
	float masses[];
};

// Another SSBO with different types
layout(std430, binding = 1) buffer TransformBuffer {
	mat4 transforms[16];
	int count;
	uint flags;
};

uniform mat4 u_viewprojection;
uniform mat4 u_model;

layout(location = 0) $out vec2 v_texcoord;
layout(location = 1) $out vec4 v_color;

void main(void) {
	v_color = a_color;
	v_texcoord = a_texcoord;
	gl_Position = u_viewprojection * u_model * vec4(a_pos.x, a_pos.y, 0.0, 1.0);
}
