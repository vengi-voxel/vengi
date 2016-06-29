const float bitScale = 255.0;
const vec4 bitSh = vec4(1.0, bitScale, bitScale * bitScale, bitScale * bitScale * bitScale);
const vec4 bitMsk = vec4(vec3(1.0 / bitScale), 0.0);
const vec4 bitShifts = vec4(1.0) / bitSh;

/**
 * float/rgba8 encoding/decoding so that we can use an RGBA8
 * shadow map instead of floating point render targets which might
 * not be supported everywhere
 *
 * http://aras-p.info/blog/2009/07/30/encoding-floats-to-rgba-the-final/
 */
vec4 encodeDepth(float v) {
	vec4 comp = fract(v * bitSh);
	comp -= comp.yzww * bitMsk;
	return comp;
}

float decodeDepth(vec4 rgba) {
	return dot(rgba, bitShifts);
}

/**
 * perform simple shadow map lookup returns 0.0 (unlit) or 1.0 (lit)
 */
float sampleShadow(sampler2D shadowMap, vec2 uv, float compare) {
	float depth = decodeDepth($texture2D(shadowMap, uv));
	depth += 0.001;
	return step(compare, depth);
}

/**
 * perform percentage-closer shadow map lookup
 */
float sampleShadowPCF(sampler2D shadowMap, vec2 uv, vec2 smSize, float compare) {
	float result = 0.0;
	for (int x = -2; x <= 2; x++) {
		for (int y = -2; y <= 2; y++) {
			vec2 off = vec2(x, y) / smSize;
			result += sampleShadow(shadowMap, uv + off, compare);
		}
	}
	return result / 25.0;
}
