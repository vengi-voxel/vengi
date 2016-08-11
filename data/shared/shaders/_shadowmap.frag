#include "_shadowsampler.frag"

#if cl_shadowmap == 1

uniform sampler2D u_shadowmap;
$in vec4 v_lightspacepos;
uniform vec2 u_screensize;

float calculateShadow(float ndotl) {
	// perform perspective divide
	vec3 lightPos = v_lightspacepos.xyz / v_lightspacepos.w;
	vec2 smUV = (lightPos.xy + 1.0) * 0.5;
	float depth = lightPos.z;
	float s = sampleShadowPCF(u_shadowmap, smUV, u_screensize, depth);
	return max(s, 0.0);
}

#else

float calculateShadow(float ndotl) {
	return 1.0;
}

#endif
