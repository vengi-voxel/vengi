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
