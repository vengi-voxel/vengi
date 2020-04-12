// attributes from the VAOs
$in vec3 a_pos;
$in uvec3 a_info;

uniform mat4 u_model;
uniform vec4 u_clipplane;
uniform mat4 u_viewprojection;
uniform sampler2D u_texture;

$out vec3 v_pos;
$out vec4 v_color;
$out vec4 v_clipspace;
$out float v_ambientocclusion;

#include "_material.vert"
#include "_fog.vert"
#include "_shadowmap.vert"
#include "_ambientocclusion.vert"

void main(void) {
	uint a_ao = a_info[0];
	uint a_colorindex = a_info[1];
	uint a_material = a_info[2];
	vec4 pos = u_model * vec4(a_pos, 1.0);
	v_pos = pos.xyz;
	v_clipspace = u_viewprojection * pos;

	gl_ClipDistance[0] = dot(pos, u_clipplane);

	int materialColorIndex = int(a_colorindex);
	vec3 materialColor = u_materialcolor[materialColorIndex].rgb;
	vec3 colornoise = texture(u_texture, abs(pos.xz) / 256.0 / 10.0).rgb;
	float alpha = u_materialcolor[a_colorindex].a;
	v_color = clamp(vec4(materialColor * colornoise * 1.8, alpha), 0.0, 1.0);
	v_ambientocclusion = aovalues[a_ao];

#if cl_shadowmap == 1
	v_lightspacepos = v_pos;
	v_viewz = (u_viewprojection * vec4(v_lightspacepos, 1.0)).w;
#endif // cl_shadowmap

	gl_Position = u_viewprojection * pos;
}
