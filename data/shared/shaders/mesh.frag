$in vec3 v_pos;
$in vec3 v_norm;
$in vec2 v_texcoords;
$in vec3 v_lightpos;
$in vec4 v_color;
uniform sampler2D u_texture;

uniform vec3 u_lightpos;
uniform vec3 u_diffuse_color;
uniform float u_fogrange;
uniform float u_viewdistance;
$out vec4 o_color;

#include "_shadowmap.frag"

void main(void) {
	vec3 lightdir = normalize(u_lightpos - v_pos);
	vec3 color = $texture2D(u_texture, v_texcoords).rgb + v_color.rgb;
	float shadow = calculateShadow();

	float ndotl = dot(v_norm, lightdir);
	vec3 diffuse = u_diffuse_color * clamp(ndotl, 0.0, 1.0) * 0.8;
	vec3 ambient = vec3(0.2);
	vec3 lightvalue = (ambient + shadow) * diffuse;

	float fogstart = max(u_viewdistance - u_fogrange, 0.0);
	float fogdistance = gl_FragCoord.z / gl_FragCoord.w;
	float fogval = 1.0 - clamp((u_viewdistance - fogdistance) / (u_viewdistance - fogstart), 0.0, 1.0);
	vec3 fogcolor = vec3(0.0, 0.6, 0.796);
	o_color = vec4(mix(color * lightvalue, fogcolor, fogval), 1.0);
}
