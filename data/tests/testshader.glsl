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
