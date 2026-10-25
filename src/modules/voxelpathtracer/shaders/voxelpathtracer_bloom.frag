in vec2 v_texcoord;

layout(binding = 0) uniform sampler2D u_texture;
layout(std140, binding = 0) uniform u_filter {
	vec2 u_resolution;
	float u_exposure;
};
layout(location = 0) out vec4 o_fragcolor;

void main(void) {
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	ivec2 maximum = ivec2(u_resolution) - 1;
	vec3 highlights = vec3(0.0);
	float totalWeight = 0.0;
	// Horizontal pass of a separable Gaussian blur; the display shader applies the vertical pass.
	// https://developer.nvidia.com/gpugems/gpugems2/part-iii-high-quality-rendering/chapter-27-advanced-high-quality-filtering
	// Apply the nonlinear highlight threshold before either blur pass.
	for (int x = -3; x <= 3; ++x) {
		float weight = exp(-0.22 * float(x * x));
		ivec2 source = clamp(pixel + ivec2(x * 3, 0), ivec2(0), maximum);
		vec3 hdr = texelFetch(u_texture, source, 0).rgb * exp2(u_exposure);
		float luminance = max(hdr.r, max(hdr.g, hdr.b));
		highlights += max(hdr - vec3(0.75), vec3(0.0)) * smoothstep(0.75, 2.5, luminance) * weight;
		totalWeight += weight;
	}
	o_fragcolor = vec4(highlights / totalWeight, 1.0);
}
