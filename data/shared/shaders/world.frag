#include "_shadow.frag"

$in vec3 v_pos;
$in vec4 v_color;
$in float v_ambientocclusion;
$in float v_debug_color;

#if cl_shadowmap == 1
uniform sampler2D u_shadowmap;
$in vec4 v_lightspacepos;
#endif

#if cl_deferred == 0
uniform vec2 u_screensize;
$in vec3 v_lightpos;
$in vec3 v_diffuse_color;
$in float v_fogrange;
$in vec3 v_fogcolor;
$in float v_viewdistance;
$out vec4 o_color;
#else
// the order is defines in the gbuffer bindings
$out vec3 o_pos;
$out vec3 o_color;
$out vec3 o_norm;
#endif

#if cl_shadowmap == 1
float calculateShadow() {
	// perform perspective divide
	vec3 lightPos = v_lightspacepos.xyz / v_lightspacepos.w;
	vec2 smUV = (lightPos.xy + 1.0) * 0.5;
	float depth = lightPos.z;
	float s = sampleShadowPCF(u_shadowmap, lightPos.xy, u_screensize, depth);
	return max(s, 0.0);
}
#endif

void main(void) {
	vec3 fdx = dFdx(v_pos.xyz);
	vec3 fdy = dFdy(v_pos.xyz);
	vec3 normal = normalize(cross(fdx, fdy));

#if cl_shadowmap == 1
	float shadow = calculateShadow();
#else
	float shadow = 0.0;
#endif

#if cl_deferred == 0
	vec3 lightdir = normalize(v_lightpos - v_pos);

	vec3 diffuse = v_diffuse_color * clamp(dot(normal, lightdir), 0.0, 1.0) * 0.8;
	vec3 ambient = vec3(0.2);
	vec3 lightvalue = ambient + (1.0 - shadow) * diffuse;

	float fogstart = max(v_viewdistance - v_fogrange, 0.0);
	float fogdistance = gl_FragCoord.z / gl_FragCoord.w;
	float fogval = 1.0 - clamp((v_viewdistance - fogdistance) / (v_viewdistance - fogstart), 0.0, 1.0);

	vec3 linearColor = v_color.rgb * v_ambientocclusion * lightvalue * v_debug_color;
	o_color = vec4(mix(linearColor, v_fogcolor, fogval), v_color.a);
#else
	o_color = v_color.xyz * v_ambientocclusion * v_debug_color;
	o_pos = v_pos;
	o_norm = normal;
#endif
}
