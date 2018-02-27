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
#if cl_fog == 1
uniform float u_fogrange;
#endif
uniform float u_viewdistance;

#include "_shadowmap.vert"

$out vec3 v_norm;
$out vec2 v_texcoords;
$out vec4 v_color;
#if cl_fog == 1
$out float v_fogdivisor;
$out float v_fogdistance;
#endif

void main(void) {
	vec4 vertexIn = vec4(a_pos, 1.0);
	vec4 normalIn = vec4(a_norm, 0.0);

	vec4 bpos =
		(u_bonetransforms[a_boneids[0]] * vertexIn) * a_boneweights[0] +
		(u_bonetransforms[a_boneids[1]] * vertexIn) * a_boneweights[1] +
		(u_bonetransforms[a_boneids[2]] * vertexIn) * a_boneweights[2] +
		(u_bonetransforms[a_boneids[3]] * vertexIn) * a_boneweights[3];
	vec4 bnorm =
		(u_bonetransforms[a_boneids[0]] * normalIn) * a_boneweights[0] +
		(u_bonetransforms[a_boneids[1]] * normalIn) * a_boneweights[1] +
		(u_bonetransforms[a_boneids[2]] * normalIn) * a_boneweights[2] +
		(u_bonetransforms[a_boneids[3]] * normalIn) * a_boneweights[3];
	vec4 mpos      = u_model * bpos;

#if cl_shadowmap == 1
	v_lightspacepos = mpos.xyz;
	v_viewz         = (u_viewprojection * vec4(v_lightspacepos, 1.0)).w;
#endif // cl_shadowmap == 1
	v_norm         = bnorm.xyz;
	v_texcoords    = a_texcoords;
	v_color        = a_color;
	gl_Position    = u_viewprojection * mpos;
#if cl_fog == 1
	v_fogdivisor   = u_viewdistance - max(u_viewdistance - u_fogrange, 0.0);
	v_fogdistance  = gl_Position.z / gl_Position.w;
#endif
}
