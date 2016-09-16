/**
 * float/rgba8 encoding/decoding so that we can use an RGBA8
 * shadow map instead of floating point render targets which might
 * not be supported everywhere
 *
 * http://aras-p.info/blog/2009/07/30/encoding-floats-to-rgba-the-final/
 * http://www.gamedev.net/topic/486847-encoding-16-and-32-bit-floating-point-value-into-rgba-byte-texture/
 *
 * Only store values between 0.0 and 1.
 */
vec4 encodeDepth(float floatValue) {
	const vec4 shift = vec4(256 * 256 * 256, 256 * 256, 256, 1.0);
	const vec4 mask = vec4(0, 1.0 / 256.0, 1.0 / 256.0, 1.0 / 256.0);
	vec4 comp = fract(floatValue * shift);
	comp -= comp.xxyz * mask;
	return comp;
}

float decodeDepth(vec4 rgba) {
	const vec4 shift = vec4(1.0 / (256.0 * 256.0 * 256.0), 1.0 / (256.0 * 256.0), 1.0 / 256.0, 1.0);
	return dot(rgba, shift);
}

/**
 * perform simple shadow map lookup returns 0.0 (unlit) or 1.0 (lit)
 * http://codeflow.org/entries/2013/feb/15/soft-shadow-mapping
 */
float sampleShadow(sampler2D shadowMap, vec2 uv, float compare, float ndotl) {
	float depth = decodeDepth($texture2D(shadowMap, uv));
	// shadow acne bias
	float bias = clamp(0.001 * tan(acos(ndotl)), 0, 0.01);
	depth += bias;
	return step(compare, depth);
}

/**
 * perform percentage-closer shadow map lookup
 * http://codeflow.org/entries/2013/feb/15/soft-shadow-mapping
 */
float sampleShadowPCF(sampler2D shadowMap, vec2 uv, vec2 smSize, float compare, float ndotl) {
	float result = 0.0;
	for (int x = -2; x <= 2; x++) {
		for (int y = -2; y <= 2; y++) {
			vec2 off = vec2(x, y) / smSize;
			result += sampleShadow(shadowMap, uv + off, compare, ndotl);
		}
	}
	return result / 25.0;
}
