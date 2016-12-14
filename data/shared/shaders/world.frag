$in vec4 v_pos;
$in vec4 v_color;
$in float v_ambientocclusion;
uniform float u_debug_color;
uniform mat4 u_viewprojection;

#include "_shadowmap.frag"

#if cl_deferred == 0
uniform vec3 u_lightdir;
uniform vec3 u_diffuse_color;
uniform vec3 u_ambient_color;
uniform vec3 u_fogcolor;
uniform float u_fogrange;
uniform float u_viewdistance;
$out vec4 o_color;
#else
// the order is defines in the gbuffer bindings
$out vec3 o_pos;
$out vec3 o_color;
$out vec3 o_norm;
#endif

void main(void) {
	vec3 fdx = dFdx(v_pos.xyz);
	vec3 fdy = dFdy(v_pos.xyz);
	vec3 normal = normalize(cross(fdx, fdy));

	float ndotl = dot(normal, u_lightdir);
	vec3 diffuse = u_diffuse_color * max(0.0, ndotl);
	vec3 ambient = u_ambient_color;

#if cl_deferred == 0
	int cascade = calculateCascade(u_viewprojection);
	float shadow = calculateShadow(cascade, u_viewprojection);
#if cl_debug_cascade
	if (cascade == 0) {
		diffuse.r = 0.0;
		diffuse.g = 1.0;
		diffuse.b = 0.0;
	} else if (cascade == 1) {
		diffuse.r = 0.0;
		diffuse.g = 1.0;
		diffuse.b = 1.0;
	} else if (cascade == 2) {
		diffuse.r = 0.0;
		diffuse.g = 0.0;
		diffuse.b = 1.0;
	} else if (cascade == 3) {
		diffuse.r = 0.0;
		diffuse.g = 0.5;
		diffuse.b = 0.5;
	} else {
		diffuse.r = 1.0;
	}
#endif
#if cl_debug_shadow == 1
	// shadow only rendering
	o_color = vec4(vec3(shadow), 1.0);
#else
	vec3 lightvalue = ambient + (diffuse * shadow);

	float fogstart = max(u_viewdistance - u_fogrange, 0.0);
	float fogdistance = gl_FragCoord.z / gl_FragCoord.w;
	float fogval = 1.0 - clamp((u_viewdistance - fogdistance) / (u_viewdistance - fogstart), 0.0, 1.0);

	vec3 linearColor = v_color.rgb * v_ambientocclusion * lightvalue * u_debug_color;
	o_color = vec4(mix(linearColor, u_fogcolor, fogval), v_color.a);
#endif
#else
	o_color = v_color.xyz * v_ambientocclusion * u_debug_color;
	o_pos = v_pos.xyz;
	o_norm = normal;
#endif
}
