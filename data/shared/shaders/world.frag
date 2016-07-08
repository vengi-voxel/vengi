#include "_shadow.frag"

$in vec3 v_pos;
$in vec4 v_color;
$in float v_ambientocclusion;
uniform float u_debug_color;

#if cl_shadowmap == 1
uniform sampler2D u_shadowmap;
$in vec4 v_lightspacepos;
uniform vec2 u_screensize;
#endif

#if cl_deferred == 0
uniform vec3 u_lightpos;
uniform vec3 u_diffuse_color;
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

#if cl_shadowmap == 1
float calculateShadow(float ndotl) {
	// perform perspective divide
	vec3 lightPos = v_lightspacepos.xyz / v_lightspacepos.w;
	vec2 smUV = (lightPos.xy + 1.0) * 0.5;
	float depth = lightPos.z;
	float s = sampleShadowPCF(u_shadowmap, smUV, u_screensize, depth);
	return max(s, 0.0);
}
#endif

void main(void) {
	vec3 fdx = dFdx(v_pos.xyz);
	vec3 fdy = dFdy(v_pos.xyz);
	vec3 normal = normalize(cross(fdx, fdy));
	vec3 lightdir = normalize(u_lightpos - v_pos);
	float ndotl = dot(normal, lightdir);
#if cl_shadowmap == 1
	float shadow = calculateShadow(ndotl);
#else
	float shadow = 1.0;
#endif

#if cl_deferred == 0
	vec3 diffuse = u_diffuse_color * clamp(ndotl, 0.0, 1.0) * 0.8;
	vec3 ambient = vec3(0.2);
	vec3 lightvalue = (ambient + shadow) * diffuse;

	float fogstart = max(u_viewdistance - u_fogrange, 0.0);
	float fogdistance = gl_FragCoord.z / gl_FragCoord.w;
	float fogval = 1.0 - clamp((u_viewdistance - fogdistance) / (u_viewdistance - fogstart), 0.0, 1.0);

	vec3 linearColor = v_color.rgb * v_ambientocclusion * lightvalue * u_debug_color;
	o_color = vec4(mix(linearColor, v_fogcolor, fogval), v_color.a);
#else
	o_color = v_color.xyz * v_ambientocclusion * u_debug_color;
	o_pos = v_pos;
	o_norm = normal;
#endif
}
