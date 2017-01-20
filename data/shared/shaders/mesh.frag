$in vec3 v_norm;
$in vec2 v_texcoords;
$in vec4 v_color;

$out vec4 o_color;

uniform sampler2D u_texture;
uniform vec3 u_lightdir;
uniform vec3 u_diffuse_color;
uniform vec3 u_ambient_color;
uniform float u_fogrange;
uniform vec3 u_fogcolor;
uniform float u_viewdistance;
uniform mat4 u_viewprojection;

#include "_shadowmap.frag"

void main(void) {
	vec3 color = $texture2D(u_texture, v_texcoords).rgb + v_color.rgb;
	int cascade = calculateCascade();
	float shadow = calculateShadow(cascade, u_viewprojection);
#if cl_debug_cascade
	if (cascade == 0) {
		color.r = 0.0;
		color.g = 1.0;
		color.b = 0.0;
	} else if (cascade == 1) {
		color.r = 0.0;
		color.g = 1.0;
		color.b = 1.0;
	} else if (cascade == 2) {
		color.r = 0.0;
		color.g = 0.0;
		color.b = 1.0;
	} else if (cascade == 3) {
		color.r = 0.0;
		color.g = 0.5;
		color.b = 0.5;
	} else {
		color.r = 1.0;
	}
#endif // cl_debug_cascade
#if cl_debug_shadow == 1
	// shadow only rendering
	o_color = vec4(vec3(shadow), 1.0);
#else // cl_debug_shadow
	float ndotl = dot(v_norm, u_lightdir);
	vec3 diffuse = u_diffuse_color * clamp(ndotl, 0.0, 1.0) * 0.8;
	vec3 ambient = u_ambient_color;
	vec3 lightvalue = ambient + (diffuse * shadow);

	float fogstart = max(u_viewdistance - u_fogrange, 0.0);
	float fogdistance = gl_FragCoord.z / gl_FragCoord.w;
	float fogval = 1.0 - clamp((u_viewdistance - fogdistance) / (u_viewdistance - fogstart), 0.0, 1.0);

	// TODO: there is an error in the fog computation - right now everything is in u_fogcolor because fogval == 1
	// this is due to the fact that fogdistance is a very very high value because gl_FragCoord.w is 0
	o_color = vec4(mix(color * lightvalue, u_fogcolor, fogval), 1.0);
#endif
}
