#include "_shadowsampler.frag"

#if cl_shadowmap == 1

// 0 floating point stuff
// 1 depth attachment
// 2 depth attachment with depth comparision (sampler2DShadow is needed)

#if cl_depthmapformat == 2
uniform sampler2DShadow u_shadowmap1;
uniform sampler2DShadow u_shadowmap2;
#else
uniform sampler2D u_shadowmap1;
uniform sampler2D u_shadowmap2;
#endif
$in vec4 v_lightspacepos;
$in vec4 v_texcoord1;
$in vec4 v_texcoord2;
uniform vec2 u_depthsize;

vec2 calculateShadowUV() {
	// convert from -1, 1 to tex coords in the range 0, 1
	vec2 uv = v_lightspacepos.xy / v_lightspacepos.w;
	return uv * 0.5 + 0.5;
}

float calculateShadow(float ndotl) {
#if cl_depthmapformat > 0
#if cl_depthmapformat == 2
	float s = $texture2D(u_shadowmap[0], v_lightspacepos.xyz).r;
#else
	float s = $texture2D(u_shadowmap[0], v_lightspacepos.xy).r;
#endif
	float visibility = 0.5;
	float bias = 0.005 * tan(acos(ndotl));
	bias = clamp(bias, 0.0, 0.01);
	if (s < v_lightspacepos.z - bias) {
		visibility = 1.0;
	}
	return visibility;
#else
	vec2 texcoord1 = v_texcoord1.xy / v_texcoord1.w;
	vec2 texcoord2 = v_texcoord2.xy / v_texcoord2.w;
	bool selection0 = all(lessThan(texcoord1, vec2(0.99))) && all(greaterThan(texcoord1, vec2(0.01)));
	bool selection1 = all(lessThan(texcoord2, vec2(0.99))) && all(greaterThan(texcoord2, vec2(0.01)));
	vec2 smUV = calculateShadowUV();
	float depth = v_lightspacepos.z;
	if (selection1) {
		return sampleShadowPCF(u_shadowmap2, smUV, u_depthsize, depth, ndotl);
	}
	return sampleShadowPCF(u_shadowmap1, smUV, u_depthsize, depth, ndotl);
#endif
}

#else

vec2 calculateShadowUV() {
	return vec2(0.0, 0.0);
}

float calculateShadow(float ndotl) {
	return 1.0;
}

#endif
