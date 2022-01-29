layout(location = 0) $out vec4 o_color;
$in vec2 v_texcoord;

uniform sampler2D u_color0;
uniform sampler2D u_color1;
uniform float u_exposure;
uniform bool u_bloom;

#ifndef cl_gamma
#define cl_gamma 2.2
#endif

void main() {
	vec4 sceneColor = $texture2D(u_color0, v_texcoord);
	vec4 bloomColor = $texture2D(u_color1, v_texcoord);

	vec3 color = sceneColor.rgb;
	if (u_bloom) {
		color += bloomColor.rgb;
	}
	// apply tone mapping
	vec3 result = vec3(1.0) - exp(-color * u_exposure);
	result = pow(result, vec3(1.0 / cl_gamma));
	o_color = vec4(result, 1.0);
}
