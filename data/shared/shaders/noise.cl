__constant int PERM_MASK = 0xff;
__constant int PERM[512] = { 151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6,
		148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74,
		165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54, 65, 25,
		63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226,
		250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2,
		44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
		251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176,
		115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180, 151, 160, 137, 91, 90,
		15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62,
		94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 77,
		146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76,
		132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123, 5, 202, 38, 147,
		118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101,
		155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144,
		12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150,
		254, 138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180, };

__constant int G_MASK = 15;
__constant int G_VECSIZE = 4;
__constant float G[16 * 4] = { +1.0f, +1.0f, +0.0f, +0.0f, -1.0f, +1.0f, +0.0f, +0.0f, +1.0f, -1.0f, +0.0f, +0.0f, -1.0f, -1.0f, +0.0f, +0.0f, +1.0f, +0.0f,
		+1.0f, +0.0f, -1.0f, +0.0f, +1.0f, +0.0f, +1.0f, +0.0f, -1.0f, +0.0f, -1.0f, +0.0f, -1.0f, +0.0f, +0.0f, +1.0f, +1.0f, +0.0f, +0.0f, -1.0f, +1.0f,
		+0.0f, +0.0f, +1.0f, -1.0f, +0.0f, +0.0f, -1.0f, -1.0f, +0.0f, +1.0f, +1.0f, +0.0f, +0.0f, -1.0f, +1.0f, +0.0f, +0.0f, +0.0f, -1.0f, +1.0f, +0.0f,
		+0.0f, -1.0f, -1.0f, +0.0f };

float smooth(float t) {
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float mix1d(float a, float b, float t) {
	const float ba = b - a;
	const float tba = t * ba;
	const float atba = a + tba;
	return atba;
}

float2 mix2d(float2 a, float2 b, float t) {
	const float2 ba = b - a;
	const float2 tba = t * ba;
	const float2 atba = a + tba;
	return atba;
}

float gradient2d(int2 i, float2 v) {
	const int index = (PERM[i.x + PERM[i.y]] & G_MASK) * G_VECSIZE;
	const float2 g = (float2)(G[index + 0], G[index + 1]);
	return dot(v, g);
}

float sgnoise2d(float2 position) {
	const float2 pf = floor(position);
	const float2 fp = position - pf;
	const int2 ip = ((int2)((int) pf.x, (int) pf.y) & PERM_MASK);

	const int2 I00 = (int2)(0, 0);
	const int2 I01 = (int2)(0, 1);
	const int2 I10 = (int2)(1, 0);
	const int2 I11 = (int2)(1, 1);

	const float2 F00 = (float2)(0.0f, 0.0f);
	const float2 F01 = (float2)(0.0f, 1.0f);
	const float2 F10 = (float2)(1.0f, 0.0f);
	const float2 F11 = (float2)(1.0f, 1.0f);

	const float n00 = gradient2d(ip + I00, fp - F00);
	const float n10 = gradient2d(ip + I10, fp - F10);
	const float n01 = gradient2d(ip + I01, fp - F01);
	const float n11 = gradient2d(ip + I11, fp - F11);

	const float2 n0001 = (float2)(n00, n01);
	const float2 n1011 = (float2)(n10, n11);

	const float2 n2 = mix2d(n0001, n1011, smooth(fp.x));
	const float n = mix1d(n2.x, n2.y, smooth(fp.y));
	return n * (1.0f / 0.7f);
}

__kernel void ridgedMF(
		__global uchar4 *output,
		const float2 bias,
		const float2 scale,
		const float lacunarity,
		const float increment,
		const int octaves,
		const float amplitude)
{
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
	const int2 size = (int2)(get_global_size(0), get_global_size(1));
	const float2 position = (float2)(coord.x / (float)size.x, coord.y / (float)size.y);
	const float2 sample = (position + bias);

	float frequency = scale.x;
	float signal = 1.0f - fabs(sgnoise2d(sample * frequency));
	float value = signal * signal;

	int i = 0;
	for (; i < octaves; ++i) {
		frequency *= lacunarity;
		const float weight = clamp(signal * 0.5f, 0.0f, 1.0f);
		signal = 1.0f - fabs(sgnoise2d(sample * frequency));
		signal *= signal;
		signal *= weight;
		value += signal;
	}

	const float4 result = (float4)(value, value, value, 1.0f) * amplitude;
	const uint idx = coord.y * size.x + coord.x;
	output[idx] = convert_uchar4_sat_rte(result * 255.0f);
}
