$in vec4 v_pos;
$in vec4 v_color;
$in float v_ambientocclusion;
uniform float u_debug_color;

#include "_shadowmap.frag"

#if cl_deferred == 0
uniform vec3 u_lightdir;
uniform vec3 u_diffuse_color;
uniform vec3 u_ambient_color;
uniform float u_fogrange;
uniform float u_viewdistance;
$in vec3 v_fogcolor;
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

#if cl_deferred == 0
	float ndotl = dot(normal, -u_lightdir);
	float shadow = calculateShadow(ndotl);
	vec3 diffuse = u_diffuse_color * clamp(ndotl, 0.0, 1.0) * 0.8;
	vec3 ambient = u_ambient_color;
	vec3 lightvalue = ambient + (shadow * diffuse);

	float fogstart = max(u_viewdistance - u_fogrange, 0.0);
	float fogdistance = gl_FragCoord.z / gl_FragCoord.w;
	float fogval = 1.0 - clamp((u_viewdistance - fogdistance) / (u_viewdistance - fogstart), 0.0, 1.0);

	vec3 linearColor = v_color.rgb * v_ambientocclusion * lightvalue * u_debug_color;
	o_color = vec4(mix(linearColor, v_fogcolor, fogval), v_color.a);
#else
	o_color = v_color.xyz * v_ambientocclusion * u_debug_color;
	o_pos = v_pos.xyz;
	o_norm = normal;
#endif
}
