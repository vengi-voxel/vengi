#include "_shadowsampler.frag"

#if cl_shadowmap == 1

uniform sampler2D u_shadowmap;
$in vec4 v_lightspacepos;
uniform vec2 u_screensize;

vec2 calculateShadowUV() {
	// convert from -1, 1 to tex coords in the range 0, 1
	return v_lightspacepos.xy * 0.5 + 0.5;
}

float calculateShadow(float ndotl) {
	vec2 smUV = calculateShadowUV();
	float depth = v_lightspacepos.z;
	return sampleShadowPCF(u_shadowmap, smUV, u_screensize, depth, ndotl);
}

#else

vec2 calculateShadowUV() {
	return vec2(0.0, 0.0);
}

float calculateShadow(float ndotl) {
	return 1.0;
}

#endif
