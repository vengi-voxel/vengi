#include "_common.frag"

#if cl_shadowmap == 1

uniform sampler2DArray u_shadowmap;
$in vec4 v_lightspacepos;
uniform vec2 u_depthsize;
uniform vec4 u_distances;
uniform mat4 u_cascades[4];

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
float sampleShadowPCF(int cascade, vec2 uv, float compare, float ndotl) {
	float result = 0.0;
	for (int x = -2; x <= 2; x++) {
		for (int y = -2; y <= 2; y++) {
			vec2 off = vec2(x, y) / u_depthsize;
			result += sampleShadow(cascade, uv + off, compare, ndotl);
		}
	}
	return result / 25.0;
}

float calculateShadow(float ndotl) {
	float viewz = v_lightspacepos.w;
	int cascade = int(dot(vec4(greaterThan(vec4(viewz), u_distances)), vec4(1)));
	vec4 lightp = u_cascades[cascade] * v_pos;
	vec3 lightpt = (lightp.xyz / lightp.w) * 0.5 + 0.5;
	return sampleShadowPCF(cascade, lightpt.xy, lightp.z / lightp.w, ndotl);
}

#else // cl_shadowmap == 1

float calculateShadow(float ndotl) {
	return 1.0;
}

#endif // cl_shadowmap == 1
