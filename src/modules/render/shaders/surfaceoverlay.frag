layout(binding = 0) uniform sampler2D u_surfaces;
layout(binding = 1) uniform sampler2D u_surfaceDepth;
layout(binding = 2) uniform sampler2D u_sceneDepth;
layout(std140, binding = 0) uniform u_frag {
	vec4 u_tint;
};

layout(location = 0) out vec4 o_color;

float surfaceSlope(ivec2 pixel, ivec2 axis, float depth) {
	ivec2 upper = textureSize(u_surfaceDepth, 0) - 1;
	float before = texelFetch(u_surfaceDepth, clamp(pixel - axis, ivec2(0), upper), 0).r;
	float after = texelFetch(u_surfaceDepth, clamp(pixel + axis, ivec2(0), upper), 0).r;
	// Prefer the continuous side at edges; cleared depth is not surface slope.
	if (before >= 1.0) {
		return after >= 1.0 ? 0.0 : abs(after - depth);
	}
	if (after >= 1.0) {
		return abs(depth - before);
	}
	return min(abs(depth - before), abs(after - depth));
}

void main() {
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	vec4 surface = texelFetch(u_surfaces, pixel, 0);
	if (surface.a <= 0.0) {
		discard;
	}
	float depth = texelFetch(u_surfaceDepth, pixel, 0).r;
	float scene = texelFetch(u_sceneDepth, pixel, 0).r;
	// Resolved MSAA scene depth may come from anywhere within the pixel rather
	// than its center. Allow that half-pixel footprint along a sloped surface.
	// Both depths are window depths, independent of the clip-space depth range.
	float footprint = 0.5 * (surfaceSlope(pixel, ivec2(1, 0), depth) + surfaceSlope(pixel, ivec2(0, 1), depth));
	float tolerance = footprint + max(abs(scene) * 4.0e-7, 1.0e-7);
	if (depth > scene + tolerance) {
		discard;
	}
	if (u_tint.a > 0.0) {
		float hatch = step(4.0, mod(gl_FragCoord.x + gl_FragCoord.y, 8.0));
		vec3 ink = mix(u_tint.rgb, vec3(1.0), hatch * 0.35);
		o_color = vec4(ink, u_tint.a * surface.a);
	} else {
		o_color = vec4(surface.rgb, surface.a * 0.9);
	}
}
