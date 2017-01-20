// attributes from the VAOs
$in vec3 a_pos;
$in vec3 a_norm;
$in vec2 a_texcoords;
$in ivec4 a_boneids;
$in vec4 a_boneweights;
$in vec4 a_color;

uniform mat4 u_viewprojection;
uniform mat4 u_model;
uniform mat4 u_bonetransforms[100];

#include "_shadowmap.vert"

$out vec3 v_norm;
$out vec2 v_texcoords;
$out vec4 v_color;

void main(void) {
	mat4 bonetrans = u_bonetransforms[a_boneids[0]] * a_boneweights[0];
	bonetrans     += u_bonetransforms[a_boneids[1]] * a_boneweights[1];
	bonetrans     += u_bonetransforms[a_boneids[2]] * a_boneweights[2];
	bonetrans     += u_bonetransforms[a_boneids[3]] * a_boneweights[3];
	vec4 mpos      = u_model * bonetrans * vec4(a_pos, 1.0);

#if cl_shadowmap == 1
	v_lightspacepos = mpos.xyz;
	v_viewz         = (u_viewprojection * vec4(v_lightspacepos, 1.0)).w;
#endif // cl_shadowmap == 1
	v_norm         = vec4(bonetrans * vec4(a_norm, 0.0)).xyz;
	v_texcoords    = a_texcoords;
	v_color        = a_color;
	gl_Position    = u_viewprojection * mpos;
}
