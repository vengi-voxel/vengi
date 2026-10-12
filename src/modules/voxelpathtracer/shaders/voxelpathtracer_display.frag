$in vec2 v_texcoord;

layout(binding = 0) uniform sampler2D u_texture;
layout(binding = 1) uniform sampler2D u_bloomTexture;

layout(std140, binding = 0) uniform u_display {
	vec2 u_screensize;
	float u_exposure;
	float u_contrast;
	float u_saturation;
	float u_vignette;
	int u_tonemapmode;
	int u_checkerboard;
	float u_bloomstrength;
	int u_bloomprecomputed;
};

layout(location = 0) $out vec4 o_fragcolor;

#include "_tonemapping.glsl"

vec3 bloom(vec3 c) {
	if (u_bloomstrength <= 1e-4) {
		return c;
	}
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	ivec2 maxP = ivec2(u_screensize) - 1;
	if (u_bloomprecomputed != 0) {
		vec3 highlights = vec3(0.0);
		float totalWeight = 0.0;
		for (int y = -3; y <= 3; ++y) {
			float weight = exp(-0.22 * float(y * y));
			ivec2 source = clamp(pixel + ivec2(0, y * 3), ivec2(0), maxP);
			highlights += texelFetch(u_bloomTexture, source, 0).rgb * weight;
			totalWeight += weight;
		}
		return c + highlights / totalWeight * u_bloomstrength * 2.5;
	}
	// Direct 2D convolution when the RGBA32F bloom intermediate is unavailable.
	vec3 sum = vec3(0.0);
	float wsum = 0.0;
	for (int y = -3; y <= 3; y++) {
		for (int x = -3; x <= 3; x++) {
			float w = exp(-0.22 * float(x * x + y * y));
			ivec2 s = clamp(pixel + ivec2(x, y) * 3, ivec2(0), maxP);
			vec3 hdr = texelFetch(u_texture, s, 0).rgb * exp2(u_exposure);
			float lum = max(hdr.r, max(hdr.g, hdr.b));
			sum += max(hdr - vec3(0.75), vec3(0.0)) * w * smoothstep(0.75, 2.5, lum);
			wsum += w;
		}
	}
	return c + (sum / max(wsum, 1e-4)) * u_bloomstrength * 2.5;
}

vec3 grade(vec3 color) {
	// ITU-R BT.709 luma coefficients for perceptual greyscale.
	const vec3 lumaCoefficients = vec3(0.2126, 0.7152, 0.0722);
	float luma = dot(color, lumaCoefficients);
	color = mix(vec3(luma), color, u_saturation);
	return clamp((color - 0.5) * u_contrast + 0.5, 0.0, 1.0);
}

vec3 vignette(vec3 color) {
	vec2 uvFromCenter = v_texcoord - 0.5;
	float amount = 1.0 - u_vignette * smoothstep(0.25, 0.85, length(uvFromCenter) * 1.4);
	return color * amount;
}

vec3 checkerboard(vec3 c, inout float alpha) {
	if (u_checkerboard != 1 || alpha >= 1.0) {
		return c;
	}
	vec2 cell = floor(v_texcoord * u_screensize / 12.0);
	float checker = mod(cell.x + cell.y, 2.0) < 1.0 ? 0.20 : 0.26;
	c = mix(vec3(checker), c, alpha);
	alpha = 1.0;
	return c;
}

void main(void) {
	vec4 acc = texture(u_texture, v_texcoord);
	vec3 c = bloom(acc.rgb * exp2(u_exposure));
	c = linearToSrgb(tonemapping(c, u_tonemapmode));
	c = grade(c);
	c = vignette(c);
	float alpha = acc.a;
	c = checkerboard(c, alpha);
	o_fragcolor = vec4(c, alpha);
}
