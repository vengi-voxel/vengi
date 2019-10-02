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
uniform int u_vertexskinning;
uniform int u_boneinfluence;

#include "_fog.vert"
#include "_shadowmap.vert"

$out vec3 v_norm;
$out vec2 v_texcoords;
$out vec4 v_color;
$out vec3 v_pos;

void main(void) {
	vec4 mpos;
	if (bool(u_vertexskinning)) {
		mat4 m;
		m = u_bonetransforms[a_boneids[0]] * a_boneweights[0]
		  + u_bonetransforms[a_boneids[1]] * a_boneweights[1]
		  + u_bonetransforms[a_boneids[2]] * a_boneweights[2]
		  + u_bonetransforms[a_boneids[3]] * a_boneweights[3];
		vec4 vertex = m * vec4(a_pos, 1.0);
		vec4 normal = m * vec4(a_norm, 0.0);
		mpos        = u_model * vertex;
		v_norm      = normal.xyz;
	} else {
		mpos        = u_model * vec4(a_pos, 1.0);
		v_norm      = a_norm;
	}

#if cl_shadowmap == 1
	v_lightspacepos = mpos.xyz;
	v_viewz         = (u_viewprojection * vec4(v_lightspacepos, 1.0)).w;
#endif // cl_shadowmap == 1

	v_texcoords    = a_texcoords;
	if (u_boneinfluence >= 0 && u_boneinfluence < 4) {
		v_color    = vec4(1.0 - a_boneweights[u_boneinfluence], a_boneweights[u_boneinfluence], 0, 1);
	} else {
		v_color    = a_color;
	}
	v_pos = mpos.xyz;
	gl_Position    = u_viewprojection * mpos;
}
