// attributes from the VAOs
$in vec3 a_pos;
$in vec3 a_norm;
$in vec2 a_texcoords;
$in ivec4 a_boneids;
$in vec4 a_weights;

uniform mat4 u_projection;
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_bonetransforms[100];

$out vec3 v_pos;
$out vec3 v_norm;
$out vec2 v_texcoords;

void main(void) {
	mat4 bonetrans = u_bonetransforms[a_boneids[0]] * a_weights[0];
	bonetrans     += u_bonetransforms[a_boneids[1]] * a_weights[1];
	bonetrans     += u_bonetransforms[a_boneids[2]] * a_weights[2];
	bonetrans     += u_bonetransforms[a_boneids[3]] * a_weights[3];
	mat4 modelview = u_view * u_model;
	vec4 pos4      = modelview * bonetrans * vec4(a_pos, 1.0);
	// TODO: does this make sense without the projection applied. Afaik
	// the w components division here is to correct perspective transforms
	v_pos          = pos4.xyz / pos4.w;
	v_norm         = vec4(bonetrans * vec4(a_norm, 0.0)).xyz;
	v_texcoords    = a_texcoords;
	gl_Position    = u_projection * pos4;
}
