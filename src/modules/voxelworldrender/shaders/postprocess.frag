uniform sampler2D u_texture;
$in vec2 v_texcoord;
uniform vec4 u_color;
layout(location = 0) $out vec4 o_color;

#ifndef cl_gamma
#define cl_gamma 1.0
#endif

void main(void) {
	vec4 color = $texture2D(u_texture, v_texcoord);
	vec3 fragcolor = pow(color.rgb * u_color.rgb, vec3(1.0 / cl_gamma));
	o_color = vec4(fragcolor, 1.0);
}
