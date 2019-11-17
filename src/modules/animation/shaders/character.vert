// attributes from the VAOs
$in vec3 a_pos;
$in uint a_color_index;
$in uint a_bone_id;
$in uint a_ambient_occlusion;

uniform mat4 u_model;
uniform mat4 u_viewprojection;
uniform mat4 u_bones[16];

#define MATERIALCOLORS 256
layout(std140) uniform u_materialblock {
	vec4 u_materialcolor[MATERIALCOLORS];
};

$out vec4 v_pos;
$out vec4 v_color;
$out float v_ambientocclusion;

#include "_fog.vert"
#include "_shadowmap.vert"
#include "_ambientocclusion.vert"

void main(void) {
	v_pos = u_model * u_bones[a_bone_id] * vec4(a_pos, 1.0);
	int materialColorIndex = int(a_color_index);
	vec3 materialColor = u_materialcolor[materialColorIndex % MATERIALCOLORS].rgb;
	v_color = vec4(materialColor, 1.0);

	v_ambientocclusion = aovalues[a_ambient_occlusion];

#if cl_shadowmap == 1
	v_lightspacepos = v_pos.xyz;
	v_viewz = (u_viewprojection * vec4(v_lightspacepos, 1.0)).w;
#endif // cl_shadowmap

	gl_Position = u_viewprojection * v_pos;
}
