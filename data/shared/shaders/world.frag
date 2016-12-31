$in vec4 v_pos;
$in vec4 v_color;
$in float v_ambientocclusion;
uniform mat4 u_viewprojection;

#include "_shadowmap.frag"

#if cl_deferred == 0
uniform vec3 u_lightdir;
uniform vec3 u_diffuse_color;
uniform vec3 u_ambient_color;
uniform vec3 u_fogcolor;
uniform float u_viewdistance;
$out vec4 o_color;
$in float v_fogdivisor;
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

	vec3 color = v_color.rgb;

#if cl_deferred == 0
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
#endif
#if cl_debug_shadow == 1
	// shadow only rendering
	o_color = vec4(vec3(shadow), 1.0);
#else
	vec3 lightvalue = u_ambient_color + (diffuse * shadow);

	float fogdistance = gl_FragCoord.z / gl_FragCoord.w;
	float fogval = 1.0 - clamp((u_viewdistance - fogdistance) / v_fogdivisor, 0.0, 1.0);

	vec3 linearColor = color * v_ambientocclusion * lightvalue;
	o_color = vec4(mix(linearColor, u_fogcolor, fogval), v_color.a);
#endif
#else
	o_color = v_color.xyz * v_ambientocclusion;
	o_pos = v_pos.xyz;
	o_norm = normal;
#endif
}
