#include "_common.frag"

#if cl_shadowmap == 1

uniform sampler2DArray u_shadowmap;
$in vec4 v_lightspacepos;
$in vec4 v_texcoord1;
$in vec4 v_texcoord2;
uniform vec2 u_depthsize;

/**
 * perform simple shadow map lookup returns 0.0 (unlit) or 1.0 (lit)
 * http://codeflow.org/entries/2013/feb/15/soft-shadow-mapping
 */
float sampleShadow(int cascade, vec2 uv, float compare, float ndotl) {
	float depth = RGBAToFloat($texture2D(u_shadowmap, vec3(uv, cascade)));
	// shadow acne bias
	float bias = clamp(0.001 * tan(acos(ndotl)), 0, 0.01);
	depth += bias;
	return step(compare, depth);
}

/**
 * perform percentage-closer shadow map lookup
 * http://codeflow.org/entries/2013/feb/15/soft-shadow-mapping
 */
float sampleShadowPCF(int cascade, vec2 uv, vec2 smSize, float compare, float ndotl) {
	float result = 0.0;
	for (int x = -2; x <= 2; x++) {
		for (int y = -2; y <= 2; y++) {
			vec2 off = vec2(x, y) / smSize;
			result += sampleShadow(cascade, uv + off, compare, ndotl);
		}
	}
	return result / 25.0;
}

vec2 calculateShadowUV() {
	// convert from -1, 1 to tex coords in the range 0, 1
	vec2 uv = v_lightspacepos.xy / v_lightspacepos.w;
	return uv * 0.5 + 0.5;
}

float calculateShadow(float ndotl) {
	vec2 texcoord1 = v_texcoord1.xy / v_texcoord1.w;
	vec2 texcoord2 = v_texcoord2.xy / v_texcoord2.w;
	bool selection0 = all(lessThan(texcoord1, vec2(0.99))) && all(greaterThan(texcoord1, vec2(0.01)));
	bool selection1 = all(lessThan(texcoord2, vec2(0.99))) && all(greaterThan(texcoord2, vec2(0.01)));
	vec2 smUV = calculateShadowUV();
	float depth = v_lightspacepos.z;
	if (selection1) {
		return sampleShadowPCF(1, smUV, u_depthsize, depth, ndotl);
	}
	return sampleShadowPCF(0, smUV, u_depthsize, depth, ndotl);
}

#else // cl_shadowmap == 1

float calculateShadow(float ndotl) {
	return 1.0;
}

#endif // cl_shadowmap == 1
