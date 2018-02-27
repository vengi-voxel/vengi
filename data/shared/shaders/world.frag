$in vec4 v_pos;
$in vec4 v_color;
$in float v_ambientocclusion;
uniform mat4 u_viewprojection;

#include "_shadowmap.frag"

uniform mediump vec3 u_lightdir;
uniform lowp vec3 u_diffuse_color;
uniform lowp vec3 u_ambient_color;
#if cl_fog == 1
uniform lowp vec3 u_fogcolor;
uniform float u_viewdistance;
#endif // cl_fog
$out vec4 o_color;
#if cl_fog == 1
$in float v_fogdivisor;
#endif // cl_fog

void main(void) {
	vec3 fdx = dFdx(v_pos.xyz);
	vec3 fdy = dFdy(v_pos.xyz);
	vec3 normal = normalize(cross(fdx, fdy));

	float ndotl = dot(normal, u_lightdir);
	vec3 diffuse = u_diffuse_color * max(0.0, ndotl);

	vec3 color = v_color.rgb;

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
	vec3 lightvalue = u_ambient_color + (diffuse * shadow);
	vec3 linearColor = color * v_ambientocclusion * lightvalue;

#if cl_fog == 1
	float fogdistance = gl_FragCoord.z / gl_FragCoord.w;
	float fogval = 1.0 - clamp((u_viewdistance - fogdistance) / v_fogdivisor, 0.0, 1.0);
	o_color = vec4(mix(linearColor, u_fogcolor, fogval), v_color.a);
#else // cl_fog
	o_color = vec4(linearColor, v_color.a);
#endif // cl_fog
#endif // cl_debug_shadow
}
