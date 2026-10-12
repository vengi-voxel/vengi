// Sobol camera samples with separate digital shifts for each pixel and sampling domain.
// https://pbr-book.org/4ed/Sampling_and_Reconstruction/Sobol_Samplers
vec2 cameraSobol(uint pixelSeed, int sampleIndex, uint domain) {
	uint seed = pcgHash(pixelSeed ^ pcgHash(domain));
	uvec2 net = uvec2(u_camerasamples[sampleIndex].xy);
	uvec2 shifted = net ^ (uvec2(seed, pcgHash(seed)) >> 8u);
	return vec2(shifted) * (1.0 / 16777216.0);
}
