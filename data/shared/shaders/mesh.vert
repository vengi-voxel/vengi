// attributes from the VAOs
$in vec3 a_pos;
$in vec3 a_norm;
$in vec2 a_texcoords;
$in ivec4 a_boneids;
$in vec4 a_weights;

uniform mat4 u_projection;
uniform mat4 u_model;
uniform mat4 u_view;
uniform float u_fogrange;
uniform float u_viewdistance;
uniform vec3 u_lightpos;
uniform mat4 u_bonetransforms[100];

$out vec3 v_pos;
$out vec3 v_norm;
$out vec2 v_texcoords;
$out vec3 v_lightpos;
$out float v_fogrange;
$out float v_viewdistance;

void main(void) {
	mat4 bonetrans = u_bonetransforms[a_boneids[0]] * a_weights[0];
	bonetrans     += u_bonetransforms[a_boneids[1]] * a_weights[1];
	bonetrans     += u_bonetransforms[a_boneids[2]] * a_weights[2];
	bonetrans     += u_bonetransforms[a_boneids[3]] * a_weights[3];
	mat4 modelview = u_view * u_model;
	vec4 pos4 = modelview * bonetrans * vec4(a_pos, 1.0);
	v_pos = vec3(pos4) / pos4.w;
	v_norm = a_norm;
	v_texcoords = a_texcoords;
	v_fogrange = u_fogrange;
	v_viewdistance = u_viewdistance;
	v_lightpos = u_lightpos;
	gl_Position = u_projection * modelview * vec4(a_pos, 1.0);
}
