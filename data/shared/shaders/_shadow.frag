vec4 encodeDepth(float v) {
	vec4 enc = vec4(1.0, 255.0, 65025.0, 160581375.0) * v;
	enc = fract(enc);
	enc -= enc.yzww * vec4(1.0 / 255.0, 1.0 / 255.0, 1.0 / 255.0, 0.0);
	return enc;
}

float decodeDepth(vec4 rgba) {
	return dot(rgba, vec4(1.0, 1.0 / 255.0, 1.0 / 65025.0, 1.0 / 160581375.0));
}

float sampleShadow(sampler2D shadowMap, vec2 uv, float compare) {
	float depth = decodeDepth($texture2D(shadowMap, uv));
	depth += 0.001;
	return step(compare, depth);
}

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
