uint g_rng;

// PCG RXS-M-XS 32-bit. Jarzynski & Olano, "Hash Functions for GPU Rendering", JCGT 2020.
// https://jcgt.org/published/0009/03/02/
uint pcgPermute(uint state) {
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

uint pcgHash(uint v) {
	return pcgPermute(v * 747796405u + 2891336453u);
}

uint pcgNext() {
	g_rng = g_rng * 747796405u + 2891336453u;
	return pcgPermute(g_rng);
}

float nextRand() {
	return float(pcgNext()) * (1.0 / 4294967296.0);
}

// Nested PCG hashes mix x, y, and frame independently. A linear combination
// (x * A + y * B + frame * C) leaves adjacent pixels with nearby LCG states, so
// the first draws are spatially correlated unless dummy pcgNext() steps scramble
// them. Hashing the seed makes those warmup calls unnecessary. This PCG variant
// also does not need an odd state (| 1u).
void seedRng(ivec2 pixel, uint frame) {
	g_rng = pcgHash(uint(pixel.x) + pcgHash(uint(pixel.y) + pcgHash(frame)));
}
