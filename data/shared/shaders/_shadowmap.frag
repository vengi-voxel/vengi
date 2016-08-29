#include "_shadowsampler.frag"

#if cl_shadowmap == 1

uniform sampler2D u_shadowmap;
$in vec4 v_lightspacepos;
uniform vec2 u_screensize;

vec2 calculateShadowUV() {
	// perform perspective divide
	vec3 lightPos = v_lightspacepos.xyz / v_lightspacepos.w;
	// convert from -1, 1 to tex coords in the range 0, 1
	vec2 smUV = (lightPos.xy + 1.0) * 0.5;
	return smUV;
}

float calculateShadow() {
	// perform perspective divide
	vec3 lightPos = v_lightspacepos.xyz / v_lightspacepos.w;
	// convert from -1, 1 to tex coords in the range 0, 1
	vec2 smUV = (lightPos.xy + 1.0) * 0.5;
	float depth = lightPos.z;
	float s = sampleShadowPCF(u_shadowmap, smUV, u_screensize, depth);
	return max(s, 0.0);
}

#else

vec2 calculateShadowUV() {
	return vec2(0.0, 0.0);
}

float calculateShadow() {
	return 1.0;
}

#endif
