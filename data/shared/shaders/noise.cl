/*
 * Permutation table. This is just a random jumble of all numbers 0-255,
 * repeated twice to avoid wrapping the index at 255 for each lookup.
 * This needs to be exactly the same for all instances on all platforms,
 * so it's easiest to just keep it as __constant explicit data.
 * This also removes the need for any initialisation of this class.
 *
 * Note that making this an int[] instead of a char[] might make the
 * code run faster on platforms with a high penalty for unaligned single
 * byte addressing. Intel x86 is generally single-byte-friendly, but
 * some other CPUs are faster with 4-aligned reads.
 * However, a char[] is smaller, which avoids cache trashing, and that
 * is probably the most important aspect on most architectures.
 * This array is accessed a *lot* by the noise functions.
 * A vector-valued noise over 3D accesses it 96 times, and a
 * float-valued 4D noise 64 times. We want this to fit in the cache!
 */
__constant uchar perm[512] = { 151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190,
		6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175,
		74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54, 65,
		25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217,
		226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152,
		2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
		251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176,
		115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180, 151, 160, 137, 91, 90,
		15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62,
		94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 77,
		146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76,
		132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123, 5, 202, 38, 147,
		118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101,
		155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144,
		12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150,
		254, 138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180 };

/*
 * Gradient tables. These could be programmed the Ken Perlin way with
 * some clever bit-twiddling, but this is more clear, and not really slower.
 */
__constant float2 grad2lut[8] = {
		(float2)(-1.0f, -1.0f),
		(float2)( 1.0f, 0.0f ),
		(float2)( -1.0f, 0.0f ),
		(float2)( 1.0f, 1.0f ),
		(float2)( -1.0f, 1.0f ),
		(float2)( 0.0f, -1.0f ),
		(float2)( 0.0f, 1.0f ),
		(float2)( 1.0f, -1.0f )
};

/*
 * Gradient directions for 3D.
 * These vectors are based on the midpoints of the 12 edges of a cube.
 * A larger array of random unit length vectors would also do the job,
 * but these 12 (including 4 repeats to make the array length a power
 * of two) work better. They are not random, they are carefully chosen
 * to represent a small, isotropic set of directions.
 */

__constant float3 grad3lut[16] = {
		(float3)( 1.0f, 0.0f, 1.0f ),
		(float3)( 0.0f, 1.0f, 1.0f ), // 12 cube edges
		(float3)( -1.0f, 0.0f, 1.0f ),
		(float3)( 0.0f, -1.0f, 1.0f ),
		(float3)( 1.0f, 0.0f, -1.0f ),
		(float3)( 0.0f, 1.0f, -1.0f ),
		(float3)( -1.0f, 0.0f, -1.0f ),
		(float3)( 0.0f, -1.0f, -1.0f ),
		(float3)( 1.0f, -1.0f, 0.0f ),
		(float3)( 1.0f, 1.0f, 0.0f ),
		(float3)( -1.0f, 1.0f, 0.0f ),
		(float3)( -1.0f, -1.0f, 0.0f ),
		(float3)( 1.0f, 0.0f, 1.0f ),
		(float3)( -1.0f, 0.0f, 1.0f ), // 4 repeats to make 16
		(float3)( 0.0f, 1.0f, -1.0f ),
		(float3)( 0.0f, -1.0f, -1.0f )
};

__constant float4 grad4lut[32] = {
		(float4)( 0.0f, 1.0f, 1.0f, 1.0f ),
		(float4)( 0.0f, 1.0f, 1.0f, -1.0f ),
		(float4)( 0.0f, 1.0f, -1.0f, 1.0f ),
		(float4)( 0.0f, 1.0f, -1.0f, -1.0f ), // 32 tesseract edges
		(float4)( 0.0f, -1.0f, 1.0f, 1.0f ),
		(float4)( 0.0f, -1.0f, 1.0f, -1.0f ),
		(float4)( 0.0f, -1.0f, -1.0f, 1.0f ),
		(float4)( 0.0f, -1.0f, -1.0f, -1.0f ),
		(float4)( 1.0f, 0.0f, 1.0f, 1.0f ),
		(float4)( 1.0f, 0.0f, 1.0f, -1.0f ),
		(float4)( 1.0f, 0.0f, -1.0f, 1.0f ),
		(float4)( 1.0f, 0.0f, -1.0f, -1.0f ),
		(float4)( -1.0f, 0.0f, 1.0f, 1.0f ),
		(float4)( -1.0f, 0.0f, 1.0f, -1.0f ),
		(float4)( -1.0f, 0.0f, -1.0f, 1.0f ),
		(float4)( -1.0f, 0.0f, -1.0f, -1.0f ),
		(float4)( 1.0f, 1.0f, 0.0f, 1.0f ),
		(float4)( 1.0f, 1.0f, 0.0f, -1.0f ),
		(float4)( 1.0f, -1.0f, 0.0f, 1.0f ),
		(float4)( 1.0f, -1.0f, 0.0f, -1.0f ),
		(float4)( -1.0f, 1.0f, 0.0f, 1.0f ),
		(float4)( -1.0f, 1.0f, 0.0f, -1.0f ),
		(float4)( -1.0f, -1.0f, 0.0f, 1.0f ),
		(float4)(-1.0f, -1.0f, 0.0f, -1.0f ),
		(float4)( 1.0f, 1.0f, 1.0f, 0.0f ),
		(float4)( 1.0f, 1.0f, -1.0f, 0.0f ),
		(float4)( 1.0f, -1.0f, 1.0f, 0.0f ),
		(float4)( 1.0f, -1.0f, -1.0f, 0.0f ),
		(float4)(-1.0f, 1.0f, 1.0f, 0.0f ),
		(float4)( -1.0f, 1.0f, -1.0f, 0.0f ),
		(float4)( -1.0f, -1.0f, 1.0f, 0.0f ),
		(float4)( -1.0f, -1.0f, -1.0f, 0.0f )
};

/*
 * For 3D, we define two orthogonal vectors in the desired rotation plane.
 * These vectors are based on the midpoints of the 12 edges of a cube,
 * they all rotate in their own plane and are never coincident or collinear.
 * A larger array of random vectors would also do the job, but these 12
 * (including 4 repeats to make the array length a power of two) work better.
 * They are not random, they are carefully chosen to represent a small
 * isotropic set of directions for any rotation angle.
 */

/* a = sqrt(2)/sqrt(3) = 0.816496580 */
#define a 0.81649658f

__constant float3 grad3u[16] = {
		(float3)(1.0f, 0.0f, 1.0f),
		(float3)( 0.0f, 1.0f, 1.0f ), // 12 cube edges
		(float3)( -1.0f, 0.0f, 1.0f ),
		(float3)( 0.0f, -1.0f, 1.0f ),
		(float3)( 1.0f, 0.0f, -1.0f ),
		(float3)( 0.0f, 1.0f, -1.0f ),
		(float3)( -1.0f, 0.0f, -1.0f ),
		(float3)( 0.0f, -1.0f, -1.0f ),
		(float3)( a, a, a ),
		(float3)( -a, a, -a ),
		(float3)( -a, -a, a ),
		(float3)( a, -a, -a ),
		(float3)( -a, a, a ),
		(float3)( a, -a, a ),
		(float3)( a, -a, -a ),
		(float3)( -a, a, -a )
};

__constant float3 grad3v[16] = {
		(float3)( -a, a, a ),
		(float3)( -a, -a, a ),
		(float3)( a, -a, a ),
		(float3)( a, a, a ),
		(float3)( -a, -a, -a ),
		(float3)( a, -a, -a ),
		(float3)( a, a, -a ),
		(float3)( -a, a, -a ),
		(float3)( 1.0f, -1.0f, 0.0f ),
		(float3)( 1.0f, 1.0f, 0.0f ),
		(float3)( -1.0f, 1.0f, 0.0f ),
		(float3)( -1.0f, -1.0f, 0.0f ),
		(float3)( 1.0f, 0.0f, 1.0f ),
		(float3)( -1.0f, 0.0f, 1.0f ), // 4 repeats to make 16
		(float3)( 0.0f, 1.0f, -1.0f ),
		(float3)( 0.0f, -1.0f, -1.0f )
};

#undef a

//---------------------------------------------------------------------

/*
 * Helper functions to compute gradients-dot-residualvectors (1D to 4D)
 * Note that these generate gradients of more than unit length. To make
 * a close match with the value range of classic Perlin noise, the final
 * noise values need to be rescaled to fit nicely within [-1,1].
 * (The simplex noise functions as such also have different scaling.)
 * Note also that these noise functions are the most practical and useful
 * signed version of Perlin noise. To return values according to the
 * RenderMan specification from the SL noise() and pnoise() functions,
 * the noise values need to be scaled and offset to [0,1], like this:
 * float SLnoise = (SimplexNoise1234::noise(x,y,z) + 1.0) * 0.5;
 */
float _grad1(int hash, float x) {
	const int h = hash & 15;
	float grad = 1.0f + (h & 7);   // Gradient value 1.0, 2.0, ..., 8.0
	if (h & 8) {
		grad = -grad;         // Set a random sign for the gradient
	}
	return (grad * x);           // Multiply the gradient with the distance
}

float _grad2(int hash, float x, float y) {
	const int h = hash & 7;      // Convert low 3 bits of hash code
	const float u = h < 4 ? x : y;  // into 8 simple gradient directions,
	const float v = h < 4 ? y : x;  // and compute the dot product with (x,y).
	return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

float _grad3(int hash, float x, float y, float z) {
	const int h = hash & 15;     // Convert low 4 bits of hash code into 12 simple
	const float u = h < 8 ? x : y; // gradient directions, and compute dot product.
	const float v = h < 4 ? y : h == 12 || h == 14 ? x : z; // Fix repeats at h = 12 to 15
	return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

float _grad4(int hash, float x, float y, float z, float t) {
	const int h = hash & 31;      // Convert low 5 bits of hash code into 32 simple
	const float u = h < 24 ? x : y; // gradient directions, and compute dot product.
	const float v = h < 16 ? y : z;
	const float w = h < 8 ? z : t;
	return ((h & 1) ? -u : u) + ((h & 2) ? -v : v) + ((h & 4) ? -w : w);
}

/*
 * Helper functions to compute gradients in 1D to 4D
 * and gradients-dot-residualvectors in 2D to 4D.
 */
void _grad1p(int hash, float *gx) {
	const int h = hash & 15;
	*gx = 1.0f + (h & 7);   // Gradient value is one of 1.0, 2.0, ..., 8.0
	if (h & 8) {
		*gx = -*gx;   // Make half of the gradients negative
	}
}

void _grad2p(int hash, float *gx, float *gy) {
	const int h = hash & 7;
	*gx = grad2lut[h].x;
	*gy = grad2lut[h].y;
}

void _grad3p(int hash, float *gx, float *gy, float *gz) {
	const int h = hash & 15;
	*gx = grad3lut[h].x;
	*gy = grad3lut[h].y;
	*gz = grad3lut[h].z;
}

void _grad4p(int hash, float *gx, float *gy, float *gz, float *gw) {
	const int h = hash & 31;
	*gx = grad4lut[h].x;
	*gy = grad4lut[h].y;
	*gz = grad4lut[h].z;
	*gw = grad4lut[h].w;
}

/*
 * Helper functions to compute rotated gradients and
 * gradients-dot-residualvectors in 2D and 3D.
 */

void _gradrot2(int hash, float sin_t, float cos_t, float *gx, float *gy) {
	const int h = hash & 7;
	const float gx0 = grad2lut[h].x;
	const float gy0 = grad2lut[h].y;
	*gx = cos_t * gx0 - sin_t * gy0;
	*gy = sin_t * gx0 + cos_t * gy0;
}

void _gradrot3(int hash, float sin_t, float cos_t, float *gx, float *gy, float *gz) {
	const int h = hash & 15;
	const float gux = grad3u[h].x;
	const float guy = grad3u[h].y;
	const float guz = grad3u[h].z;
	const float gvx = grad3v[h].x;
	const float gvy = grad3v[h].y;
	const float gvz = grad3v[h].z;
	*gx = cos_t * gux + sin_t * gvx;
	*gy = cos_t * guy + sin_t * gvy;
	*gz = cos_t * guz + sin_t * gvz;
}

float _graddotp2(float gx, float gy, float x, float y) {
	return gx * x + gy * y;
}

float _graddotp3(float gx, float gy, float gz, float x, float y, float z) {
	return gx * x + gy * y + gz * z;
}

/* Skewing factors for 2D simplex grid:
 * F2 = 0.5*(sqrt(3.0)-1.0)
 * G2 = (3.0-Math.sqrt(3.0))/6.0
 */
#define F2 0.366025403f
#define G2 0.211324865f
/* Skewing factors for 3D simplex grid:
 * F3 = 1/3
 * G3 = 1/6 */
#define F3 0.333333333f
#define G3 0.166666667f

// The skewing and unskewing factors are hairy again for the 4D case
#define F4 0.309016994f // F4 = (Math.sqrt(5.0)-1.0)/4.0
#define G4 0.138196601f // G4 = (5.0-Math.sqrt(5.0))/20.0

float _noise1(float x) {
	const int i0 = floor(x);
	const int i1 = i0 + 1;
	const float x0 = x - i0;
	const float x1 = x0 - 1.0f;

	float t0 = 1.0f - x0 * x0;
	//  if(t0 < 0.0f) t0 = 0.0f;
	t0 *= t0;
	const float n0 = t0 * t0 * _grad1(perm[i0 & 0xff], x0);

	float t1 = 1.0f - x1 * x1;
	//  if(t1 < 0.0f) t1 = 0.0f;
	t1 *= t1;
	const float n1 = t1 * t1 * _grad1(perm[i1 & 0xff], x1);
	// The maximum value of this noise is 8*(3/4)^4 = 2.53125
	// A factor of 0.395 would scale to fit exactly within [-1,1], but
	// we want to match PRMan's 1D noise, so we scale it down some more.
	return 0.25f * (n0 + n1);

}

// 2D simplex noise
float _noise2(float2 v) {
	float n0, n1, n2; // Noise contributions from the three corners

	// Skew the input space to determine which simplex cell we're in
	const float s = (v.x + v.y) * F2; // Hairy factor for 2D
	const float xs = v.x + s;
	const float ys = v.y + s;
	const int i = floor(xs);
	const int j = floor(ys);

	const float t = (float) (i + j) * G2;
	const float X0 = i - t; // Unskew the cell origin back to (x,y) space
	const float Y0 = j - t;
	const float x0 = v.x - X0; // The x,y distances from the cell origin
	const float y0 = v.y - Y0;

	// For the 2D case, the simplex shape is an equilateral triangle.
	// Determine which simplex we are in.
	int i1, j1; // Offsets for second (middle) corner of simplex in (i,j) coords
	if (x0 > y0) {
		i1 = 1;
		j1 = 0;
	} // lower triangle, XY order: (0,0)->(1,0)->(1,1)
	else {
		i1 = 0;
		j1 = 1;
	}      // upper triangle, YX order: (0,0)->(0,1)->(1,1)

	// A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
	// a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
	// c = (3-sqrt(3))/6

	const float x1 = x0 - i1 + G2; // Offsets for middle corner in (x,y) unskewed coords
	const float y1 = y0 - j1 + G2;
	const float x2 = x0 - 1.0f + 2.0f * G2; // Offsets for last corner in (x,y) unskewed coords
	const float y2 = y0 - 1.0f + 2.0f * G2;

	// Wrap the integer indices at 256, to avoid indexing perm[] out of bounds
	const int ii = i & 0xff;
	const int jj = j & 0xff;

	// Calculate the contribution from the three corners
	float t0 = 0.5f - x0 * x0 - y0 * y0;
	if (t0 < 0.0f)
		n0 = 0.0f;
	else {
		t0 *= t0;
		n0 = t0 * t0 * _grad2(perm[ii + perm[jj]], x0, y0);
	}

	float t1 = 0.5f - x1 * x1 - y1 * y1;
	if (t1 < 0.0f)
		n1 = 0.0f;
	else {
		t1 *= t1;
		n1 = t1 * t1 * _grad2(perm[ii + i1 + perm[jj + j1]], x1, y1);
	}

	float t2 = 0.5f - x2 * x2 - y2 * y2;
	if (t2 < 0.0f)
		n2 = 0.0f;
	else {
		t2 *= t2;
		n2 = t2 * t2 * _grad2(perm[ii + 1 + perm[jj + 1]], x2, y2);
	}

	// Add contributions from each corner to get the final noise value.
	// The result is scaled to return values in the interval [-1,1].
	return 40.0f * (n0 + n1 + n2); // TODO: The scale factor is preliminary!
}

// 3D simplex noise
float _noise3(float3 v) {
	float n0, n1, n2, n3; // Noise contributions from the four corners

	// Skew the input space to determine which simplex cell we're in
	const float s = (v.x + v.y + v.z) * F3; // Very nice and simple skew factor for 3D
	const float xs = v.x + s;
	const float ys = v.y + s;
	const float zs = v.z + s;
	const int i = floor(xs);
	const int j = floor(ys);
	const int k = floor(zs);

	const float t = (float) (i + j + k) * G3;
	const float X0 = i - t; // Unskew the cell origin back to (x,y,z) space
	const float Y0 = j - t;
	const float Z0 = k - t;
	const float x0 = v.x - X0; // The x,y,z distances from the cell origin
	const float y0 = v.y - Y0;
	const float z0 = v.z - Z0;

	// For the 3D case, the simplex shape is a slightly irregular tetrahedron.
	// Determine which simplex we are in.
	int i1, j1, k1; // Offsets for second corner of simplex in (i,j,k) coords
	int i2, j2, k2; // Offsets for third corner of simplex in (i,j,k) coords

	/* This code would benefit from a backport from the GLSL version! */
	if (x0 >= y0) {
		if (y0 >= z0) {
			i1 = 1;
			j1 = 0;
			k1 = 0;
			i2 = 1;
			j2 = 1;
			k2 = 0;
		} // X Y Z order
		else if (x0 >= z0) {
			i1 = 1;
			j1 = 0;
			k1 = 0;
			i2 = 1;
			j2 = 0;
			k2 = 1;
		} // X Z Y order
		else {
			i1 = 0;
			j1 = 0;
			k1 = 1;
			i2 = 1;
			j2 = 0;
			k2 = 1;
		} // Z X Y order
	} else { // x0<y0
		if (y0 < z0) {
			i1 = 0;
			j1 = 0;
			k1 = 1;
			i2 = 0;
			j2 = 1;
			k2 = 1;
		} // Z Y X order
		else if (x0 < z0) {
			i1 = 0;
			j1 = 1;
			k1 = 0;
			i2 = 0;
			j2 = 1;
			k2 = 1;
		} // Y Z X order
		else {
			i1 = 0;
			j1 = 1;
			k1 = 0;
			i2 = 1;
			j2 = 1;
			k2 = 0;
		} // Y X Z order
	}

	// A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
	// a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
	// a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
	// c = 1/6.

	const float x1 = x0 - i1 + G3; // Offsets for second corner in (x,y,z) coords
	const float y1 = y0 - j1 + G3;
	const float z1 = z0 - k1 + G3;
	const float x2 = x0 - i2 + 2.0f * G3; // Offsets for third corner in (x,y,z) coords
	const float y2 = y0 - j2 + 2.0f * G3;
	const float z2 = z0 - k2 + 2.0f * G3;
	const float x3 = x0 - 1.0f + 3.0f * G3; // Offsets for last corner in (x,y,z) coords
	const float y3 = y0 - 1.0f + 3.0f * G3;
	const float z3 = z0 - 1.0f + 3.0f * G3;

	// Wrap the integer indices at 256, to avoid indexing perm[] out of bounds
	const int ii = i & 0xff;
	const int jj = j & 0xff;
	const int kk = k & 0xff;

	// Calculate the contribution from the four corners
	float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
	if (t0 < 0.0f)
		n0 = 0.0f;
	else {
		t0 *= t0;
		n0 = t0 * t0 * _grad3(perm[ii + perm[jj + perm[kk]]], x0, y0, z0);
	}

	float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
	if (t1 < 0.0f)
		n1 = 0.0f;
	else {
		t1 *= t1;
		n1 = t1 * t1 * _grad3(perm[ii + i1 + perm[jj + j1 + perm[kk + k1]]], x1, y1, z1);
	}

	float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
	if (t2 < 0.0f)
		n2 = 0.0f;
	else {
		t2 *= t2;
		n2 = t2 * t2 * _grad3(perm[ii + i2 + perm[jj + j2 + perm[kk + k2]]], x2, y2, z2);
	}

	float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
	if (t3 < 0.0f)
		n3 = 0.0f;
	else {
		t3 *= t3;
		n3 = t3 * t3 * _grad3(perm[ii + 1 + perm[jj + 1 + perm[kk + 1]]], x3, y3, z3);
	}

	// Add contributions from each corner to get the final noise value.
	// The result is scaled to stay just inside [-1,1]
	return 32.0f * (n0 + n1 + n2 + n3); // TODO: The scale factor is preliminary!
}

__constant uchar4 sSimplexLut[64] = {
		(uchar4)( 0, 1, 2, 3 ),
		(uchar4)( 0, 1, 3, 2 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 2, 3, 1 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 1, 2, 3, 0 ),
		(uchar4)( 0, 2, 1, 3 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 3, 1, 2 ),
		(uchar4)( 0, 3, 2, 1 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 1, 3, 2, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 1, 2, 0, 3 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 1, 3, 0, 2 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 2, 3, 0, 1 ),
		(uchar4)( 2, 3, 1, 0 ),
		(uchar4)( 1, 0, 2, 3 ),
		(uchar4)( 1, 0, 3, 2 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 2, 0, 3, 1 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 2, 1, 3, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 2, 0, 1, 3 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 3, 0, 1, 2 ),
		(uchar4)( 3, 0, 2, 1 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 3, 1, 2, 0 ),
		(uchar4)( 2, 1, 0, 3 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 3, 1, 0, 2 ),
		(uchar4)( 0, 0, 0, 0 ),
		(uchar4)( 3, 2, 0, 1 ),
		(uchar4)( 3, 2, 1, 0 )
};

// 4D simplex noise
float _noise4(float4 v) {
	float n0, n1, n2, n3, n4; // Noise contributions from the five corners

	// Skew the (x,y,z,w) space to determine which cell of 24 simplices we're in
	const float s = (v.x + v.y + v.z + v.w) * F4; // Factor for 4D skewing
	const float xs = v.x + s;
	const float ys = v.y + s;
	const float zs = v.z + s;
	const float ws = v.w + s;
	const int i = floor(xs);
	const int j = floor(ys);
	const int k = floor(zs);
	const int l = floor(ws);

	const float t = (i + j + k + l) * G4; // Factor for 4D unskewing
	const float X0 = i - t; // Unskew the cell origin back to (x,y,z,w) space
	const float Y0 = j - t;
	const float Z0 = k - t;
	const float W0 = l - t;

	const float x0 = v.x - X0;  // The x,y,z,w distances from the cell origin
	const float y0 = v.y - Y0;
	const float z0 = v.z - Z0;
	const float w0 = v.w - W0;

	// For the 4D case, the simplex is a 4D shape I won't even try to describe.
	// To find out which of the 24 possible simplices we're in, we need to
	// determine the magnitude ordering of x0, y0, z0 and w0.
	// The method below is a good way of finding the ordering of x,y,z,w and
	// then find the correct traversal order for the simplex weíre in.
	// First, six pair-wise comparisons are performed between each possible pair
	// of the four coordinates, and the results are used to add up binary bits
	// for an integer index.
	const int c1 = (x0 > y0) ? 32 : 0;
	const int c2 = (x0 > z0) ? 16 : 0;
	const int c3 = (y0 > z0) ? 8 : 0;
	const int c4 = (x0 > w0) ? 4 : 0;
	const int c5 = (y0 > w0) ? 2 : 0;
	const int c6 = (z0 > w0) ? 1 : 0;
	const int c = c1 + c2 + c3 + c4 + c5 + c6;

	int i1, j1, k1, l1; // The integer offsets for the second simplex corner
	int i2, j2, k2, l2; // The integer offsets for the third simplex corner
	int i3, j3, k3, l3; // The integer offsets for the fourth simplex corner

	// sSimplexLut[c] is a 4-vector with the numbers 0, 1, 2 and 3 in some order.
	// Many values of c will never occur, since e.g. x>y>z>w makes x<z, y<w and x<w
	// impossible. Only the 24 indices which have non-zero entries make any sense.
	// We use a thresholding to set the coordinates in turn from the largest magnitude.
	// The number 3 in the "simplex" array is at the position of the largest coordinate.
	i1 = sSimplexLut[c].x >= 3 ? 1 : 0;
	j1 = sSimplexLut[c].y >= 3 ? 1 : 0;
	k1 = sSimplexLut[c].z >= 3 ? 1 : 0;
	l1 = sSimplexLut[c].w >= 3 ? 1 : 0;
	// The number 2 in the "simplex" array is at the second largest coordinate.
	i2 = sSimplexLut[c].x >= 2 ? 1 : 0;
	j2 = sSimplexLut[c].y >= 2 ? 1 : 0;
	k2 = sSimplexLut[c].z >= 2 ? 1 : 0;
	l2 = sSimplexLut[c].w >= 2 ? 1 : 0;
	// The number 1 in the "simplex" array is at the second smallest coordinate.
	i3 = sSimplexLut[c].x >= 1 ? 1 : 0;
	j3 = sSimplexLut[c].y >= 1 ? 1 : 0;
	k3 = sSimplexLut[c].z >= 1 ? 1 : 0;
	l3 = sSimplexLut[c].w >= 1 ? 1 : 0;
	// The fifth corner has all coordinate offsets = 1, so no need to look that up.

	const float x1 = x0 - i1 + G4; // Offsets for second corner in (x,y,z,w) coords
	const float y1 = y0 - j1 + G4;
	const float z1 = z0 - k1 + G4;
	const float w1 = w0 - l1 + G4;
	const float x2 = x0 - i2 + 2.0f * G4; // Offsets for third corner in (x,y,z,w) coords
	const float y2 = y0 - j2 + 2.0f * G4;
	const float z2 = z0 - k2 + 2.0f * G4;
	const float w2 = w0 - l2 + 2.0f * G4;
	const float x3 = x0 - i3 + 3.0f * G4; // Offsets for fourth corner in (x,y,z,w) coords
	const float y3 = y0 - j3 + 3.0f * G4;
	const float z3 = z0 - k3 + 3.0f * G4;
	const float w3 = w0 - l3 + 3.0f * G4;
	const float x4 = x0 - 1.0f + 4.0f * G4; // Offsets for last corner in (x,y,z,w) coords
	const float y4 = y0 - 1.0f + 4.0f * G4;
	const float z4 = z0 - 1.0f + 4.0f * G4;
	const float w4 = w0 - 1.0f + 4.0f * G4;

	// Wrap the integer indices at 256, to avoid indexing perm[] out of bounds
	const int ii = i & 0xff;
	const int jj = j & 0xff;
	const int kk = k & 0xff;
	const int ll = l & 0xff;

	// Calculate the contribution from the five corners
	float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0 - w0 * w0;
	if (t0 < 0.0f)
		n0 = 0.0f;
	else {
		t0 *= t0;
		n0 = t0 * t0 * _grad4(perm[ii + perm[jj + perm[kk + perm[ll]]]], x0, y0, z0, w0);
	}

	float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1 - w1 * w1;
	if (t1 < 0.0f)
		n1 = 0.0f;
	else {
		t1 *= t1;
		n1 = t1 * t1 * _grad4(perm[ii + i1 + perm[jj + j1 + perm[kk + k1 + perm[ll + l1]]]], x1, y1, z1, w1);
	}

	float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2 - w2 * w2;
	if (t2 < 0.0f)
		n2 = 0.0f;
	else {
		t2 *= t2;
		n2 = t2 * t2 * _grad4(perm[ii + i2 + perm[jj + j2 + perm[kk + k2 + perm[ll + l2]]]], x2, y2, z2, w2);
	}

	float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3 - w3 * w3;
	if (t3 < 0.0f)
		n3 = 0.0f;
	else {
		t3 *= t3;
		n3 = t3 * t3 * _grad4(perm[ii + i3 + perm[jj + j3 + perm[kk + k3 + perm[ll + l3]]]], x3, y3, z3, w3);
	}

	float t4 = 0.6f - x4 * x4 - y4 * y4 - z4 * z4 - w4 * w4;
	if (t4 < 0.0f)
		n4 = 0.0f;
	else {
		t4 *= t4;
		n4 = t4 * t4 * _grad4(perm[ii + 1 + perm[jj + 1 + perm[kk + 1 + perm[ll + 1]]]], x4, y4, z4, w4);
	}

	// Sum up and scale the result to cover the range [-1,1]
	return 27.0f * (n0 + n1 + n2 + n3 + n4); // TODO: The scale factor is preliminary!
}

float2 _dnoise1(float x) {
	const int i0 = floor(x);
	const int i1 = i0 + 1;
	const float x0 = x - i0;
	const float x1 = x0 - 1.0f;

	float gx0, gx1;
	float n0, n1;
	float t20, t40, t21, t41;

	const float x20 = x0 * x0;
	const float t0 = 1.0f - x20;
	//  if(t0 < 0.0f) t0 = 0.0f; // Never happens for 1D: x0<=1 always
	t20 = t0 * t0;
	t40 = t20 * t20;
	_grad1p(perm[i0 & 0xff], &gx0);
	n0 = t40 * gx0 * x0;

	const float x21 = x1 * x1;
	const float t1 = 1.0f - x21;
	//  if(t1 < 0.0f) t1 = 0.0f; // Never happens for 1D: |x1|<=1 always
	t21 = t1 * t1;
	t41 = t21 * t21;
	_grad1p(perm[i1 & 0xff], &gx1);
	n1 = t41 * gx1 * x1;

	/* Compute derivative according to:
	 *  *dnoise_dx = -8.0f * t20 * t0 * x0 * (gx0 * x0) + t40 * gx0;
	 *  *dnoise_dx += -8.0f * t21 * t1 * x1 * (gx1 * x1) + t41 * gx1;
	 */
	float dnoise_dx = t20 * t0 * gx0 * x20;
	dnoise_dx += t21 * t1 * gx1 * x21;
	dnoise_dx *= -8.0f;
	dnoise_dx += t40 * gx0 + t41 * gx1;
	dnoise_dx *= 0.25f; /* Scale derivative to match the noise scaling */

	// The maximum value of this noise is 8*(3/4)^4 = 2.53125
	// A factor of 0.395 would scale to fit exactly within [-1,1], but
	// to better match classic Perlin noise, we scale it down some more.
	return (float2)(0.25f * (n0 + n1), dnoise_dx);
}

float3 _dnoise2(float2 v) {
	float n0, n1, n2; // Noise contributions from the three corners

	// Skew the input space to determine which simplex cell we're in
	const float s = (v.x + v.y) * F2; // Hairy factor for 2D
	const float xs = v.x + s;
	const float ys = v.y + s;
	const int i = floor(xs);
	const int j = floor(ys);

	const float t = (float) (i + j) * G2;
	const float X0 = i - t; // Unskew the cell origin back to (x,y) space
	const float Y0 = j - t;
	const float x0 = v.x - X0; // The x,y distances from the cell origin
	const float y0 = v.y - Y0;

	// For the 2D case, the simplex shape is an equilateral triangle.
	// Determine which simplex we are in.
	int i1, j1; // Offsets for second (middle) corner of simplex in (i,j) coords
	if (x0 > y0) {
		i1 = 1;
		j1 = 0;
	} // lower triangle, XY order: (0,0)->(1,0)->(1,1)
	else {
		i1 = 0;
		j1 = 1;
	}      // upper triangle, YX order: (0,0)->(0,1)->(1,1)

	// A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
	// a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
	// c = (3-sqrt(3))/6

	const float x1 = x0 - i1 + G2; // Offsets for middle corner in (x,y) unskewed coords
	const float y1 = y0 - j1 + G2;
	const float x2 = x0 - 1.0f + 2.0f * G2; // Offsets for last corner in (x,y) unskewed coords
	const float y2 = y0 - 1.0f + 2.0f * G2;

	// Wrap the integer indices at 256, to avoid indexing perm[] out of bounds
	const int ii = i & 0xff;
	const int jj = j & 0xff;

	float gx0, gy0, gx1, gy1, gx2, gy2; /* Gradients at simplex corners */

	/* Calculate the contribution from the three corners */
	float t0 = 0.5f - x0 * x0 - y0 * y0;
	float t20, t40;
	if (t0 < 0.0f)
		t40 = t20 = t0 = n0 = gx0 = gy0 = 0.0f; /* No influence */
	else {
		_grad2p(perm[ii + perm[jj]], &gx0, &gy0);
		t20 = t0 * t0;
		t40 = t20 * t20;
		n0 = t40 * (gx0 * x0 + gy0 * y0);
	}

	float t1 = 0.5f - x1 * x1 - y1 * y1;
	float t21, t41;
	if (t1 < 0.0f)
		t21 = t41 = t1 = n1 = gx1 = gy1 = 0.0f; /* No influence */
	else {
		_grad2p(perm[ii + i1 + perm[jj + j1]], &gx1, &gy1);
		t21 = t1 * t1;
		t41 = t21 * t21;
		n1 = t41 * (gx1 * x1 + gy1 * y1);
	}

	float t2 = 0.5f - x2 * x2 - y2 * y2;
	float t22, t42;
	if (t2 < 0.0f)
		t42 = t22 = t2 = n2 = gx2 = gy2 = 0.0f; /* No influence */
	else {
		_grad2p(perm[ii + 1 + perm[jj + 1]], &gx2, &gy2);
		t22 = t2 * t2;
		t42 = t22 * t22;
		n2 = t42 * (gx2 * x2 + gy2 * y2);
	}

	/* Compute derivative, if requested by supplying non-null pointers
	 * for the last two arguments */
	/*  A straight, unoptimised calculation would be like:
	 *    *dnoise_dx = -8.0f * t20 * t0 * x0 * ( gx0 * x0 + gy0 * y0 ) + t40 * gx0;
	 *    *dnoise_dy = -8.0f * t20 * t0 * y0 * ( gx0 * x0 + gy0 * y0 ) + t40 * gy0;
	 *    *dnoise_dx += -8.0f * t21 * t1 * x1 * ( gx1 * x1 + gy1 * y1 ) + t41 * gx1;
	 *    *dnoise_dy += -8.0f * t21 * t1 * y1 * ( gx1 * x1 + gy1 * y1 ) + t41 * gy1;
	 *    *dnoise_dx += -8.0f * t22 * t2 * x2 * ( gx2 * x2 + gy2 * y2 ) + t42 * gx2;
	 *    *dnoise_dy += -8.0f * t22 * t2 * y2 * ( gx2 * x2 + gy2 * y2 ) + t42 * gy2;
	 */
	const float temp0 = t20 * t0 * (gx0 * x0 + gy0 * y0);
	float dnoise_dx = temp0 * x0;
	float dnoise_dy = temp0 * y0;
	const float temp1 = t21 * t1 * (gx1 * x1 + gy1 * y1);
	dnoise_dx += temp1 * x1;
	dnoise_dy += temp1 * y1;
	const float temp2 = t22 * t2 * (gx2 * x2 + gy2 * y2);
	dnoise_dx += temp2 * x2;
	dnoise_dy += temp2 * y2;
	dnoise_dx *= -8.0f;
	dnoise_dy *= -8.0f;
	dnoise_dx += t40 * gx0 + t41 * gx1 + t42 * gx2;
	dnoise_dy += t40 * gy0 + t41 * gy1 + t42 * gy2;
	dnoise_dx *= 40.0f; /* Scale derivative to match the noise scaling */
	dnoise_dy *= 40.0f;

	// Add contributions from each corner to get the final noise value.
	// The result is scaled to return values in the interval [-1,1].
	return (float3)(40.0f * (n0 + n1 + n2), dnoise_dx, dnoise_dy); // TODO: The scale factor is preliminary!
}

float4 _dnoise3(float3 v) {
	float n0, n1, n2, n3; /* Noise contributions from the four simplex corners */
	float noise; /* Return value */
	float gx0, gy0, gz0, gx1, gy1, gz1; /* Gradients at simplex corners */
	float gx2, gy2, gz2, gx3, gy3, gz3;

	/* Skew the input space to determine which simplex cell we're in */
	const float s = (v.x + v.y + v.z) * F3; /* Very nice and simple skew factor for 3D */
	const float xs = v.x + s;
	const float ys = v.y + s;
	const float zs = v.z + s;
	const int i = floor(xs);
	const int j = floor(ys);
	const int k = floor(zs);

	const float t = (float) (i + j + k) * G3;
	const float X0 = i - t; /* Unskew the cell origin back to (x,y,z) space */
	const float Y0 = j - t;
	const float Z0 = k - t;
	const float x0 = v.x - X0; /* The x,y,z distances from the cell origin */
	const float y0 = v.y - Y0;
	const float z0 = v.z - Z0;

	/* For the 3D case, the simplex shape is a slightly irregular tetrahedron.
	 * Determine which simplex we are in. */
	int i1, j1, k1; /* Offsets for second corner of simplex in (i,j,k) coords */
	int i2, j2, k2; /* Offsets for third corner of simplex in (i,j,k) coords */

	/* TODO: This code would benefit from a backport from the GLSL version! */
	if (x0 >= y0) {
		if (y0 >= z0) {
			i1 = 1;
			j1 = 0;
			k1 = 0;
			i2 = 1;
			j2 = 1;
			k2 = 0;
		} /* X Y Z order */
		else if (x0 >= z0) {
			i1 = 1;
			j1 = 0;
			k1 = 0;
			i2 = 1;
			j2 = 0;
			k2 = 1;
		} /* X Z Y order */
		else {
			i1 = 0;
			j1 = 0;
			k1 = 1;
			i2 = 1;
			j2 = 0;
			k2 = 1;
		} /* Z X Y order */
	} else { // x0<y0
		if (y0 < z0) {
			i1 = 0;
			j1 = 0;
			k1 = 1;
			i2 = 0;
			j2 = 1;
			k2 = 1;
		} /* Z Y X order */
		else if (x0 < z0) {
			i1 = 0;
			j1 = 1;
			k1 = 0;
			i2 = 0;
			j2 = 1;
			k2 = 1;
		} /* Y Z X order */
		else {
			i1 = 0;
			j1 = 1;
			k1 = 0;
			i2 = 1;
			j2 = 1;
			k2 = 0;
		} /* Y X Z order */
	}

	/* A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
	 * a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
	 * a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
	 * c = 1/6.   */

	const float x1 = x0 - i1 + G3; /* Offsets for second corner in (x,y,z) coords */
	const float y1 = y0 - j1 + G3;
	const float z1 = z0 - k1 + G3;
	const float x2 = x0 - i2 + 2.0f * G3; /* Offsets for third corner in (x,y,z) coords */
	const float y2 = y0 - j2 + 2.0f * G3;
	const float z2 = z0 - k2 + 2.0f * G3;
	const float x3 = x0 - 1.0f + 3.0f * G3; /* Offsets for last corner in (x,y,z) coords */
	const float y3 = y0 - 1.0f + 3.0f * G3;
	const float z3 = z0 - 1.0f + 3.0f * G3;

	/* Wrap the integer indices at 256, to avoid indexing perm[] out of bounds */
	const int ii = i & 0xff;
	const int jj = j & 0xff;
	const int kk = k & 0xff;

	/* Calculate the contribution from the four corners */
	float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
	float t20, t40;
	if (t0 < 0.0f)
		n0 = t0 = t20 = t40 = gx0 = gy0 = gz0 = 0.0f;
	else {
		_grad3p(perm[ii + perm[jj + perm[kk]]], &gx0, &gy0, &gz0);
		t20 = t0 * t0;
		t40 = t20 * t20;
		n0 = t40 * (gx0 * x0 + gy0 * y0 + gz0 * z0);
	}

	float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
	float t21, t41;
	if (t1 < 0.0f)
		n1 = t1 = t21 = t41 = gx1 = gy1 = gz1 = 0.0f;
	else {
		_grad3p(perm[ii + i1 + perm[jj + j1 + perm[kk + k1]]], &gx1, &gy1, &gz1);
		t21 = t1 * t1;
		t41 = t21 * t21;
		n1 = t41 * (gx1 * x1 + gy1 * y1 + gz1 * z1);
	}

	float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
	float t22, t42;
	if (t2 < 0.0f)
		n2 = t2 = t22 = t42 = gx2 = gy2 = gz2 = 0.0f;
	else {
		_grad3p(perm[ii + i2 + perm[jj + j2 + perm[kk + k2]]], &gx2, &gy2, &gz2);
		t22 = t2 * t2;
		t42 = t22 * t22;
		n2 = t42 * (gx2 * x2 + gy2 * y2 + gz2 * z2);
	}

	float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
	float t23, t43;
	if (t3 < 0.0f)
		n3 = t3 = t23 = t43 = gx3 = gy3 = gz3 = 0.0f;
	else {
		_grad3p(perm[ii + 1 + perm[jj + 1 + perm[kk + 1]]], &gx3, &gy3, &gz3);
		t23 = t3 * t3;
		t43 = t23 * t23;
		n3 = t43 * (gx3 * x3 + gy3 * y3 + gz3 * z3);
	}

	/*  Add contributions from each corner to get the final noise value.
	 * The result is scaled to return values in the range [-1,1] */
	noise = 28.0f * (n0 + n1 + n2 + n3);

	/* Compute derivative, if requested by supplying non-null pointers
	 * for the last three arguments */
	/*  A straight, unoptimised calculation would be like:
	 *     *dnoise_dx = -8.0f * t20 * t0 * x0 * dot(gx0, gy0, gz0, x0, y0, z0) + t40 * gx0;
	 *    *dnoise_dy = -8.0f * t20 * t0 * y0 * dot(gx0, gy0, gz0, x0, y0, z0) + t40 * gy0;
	 *    *dnoise_dz = -8.0f * t20 * t0 * z0 * dot(gx0, gy0, gz0, x0, y0, z0) + t40 * gz0;
	 *    *dnoise_dx += -8.0f * t21 * t1 * x1 * dot(gx1, gy1, gz1, x1, y1, z1) + t41 * gx1;
	 *    *dnoise_dy += -8.0f * t21 * t1 * y1 * dot(gx1, gy1, gz1, x1, y1, z1) + t41 * gy1;
	 *    *dnoise_dz += -8.0f * t21 * t1 * z1 * dot(gx1, gy1, gz1, x1, y1, z1) + t41 * gz1;
	 *    *dnoise_dx += -8.0f * t22 * t2 * x2 * dot(gx2, gy2, gz2, x2, y2, z2) + t42 * gx2;
	 *    *dnoise_dy += -8.0f * t22 * t2 * y2 * dot(gx2, gy2, gz2, x2, y2, z2) + t42 * gy2;
	 *    *dnoise_dz += -8.0f * t22 * t2 * z2 * dot(gx2, gy2, gz2, x2, y2, z2) + t42 * gz2;
	 *    *dnoise_dx += -8.0f * t23 * t3 * x3 * dot(gx3, gy3, gz3, x3, y3, z3) + t43 * gx3;
	 *    *dnoise_dy += -8.0f * t23 * t3 * y3 * dot(gx3, gy3, gz3, x3, y3, z3) + t43 * gy3;
	 *    *dnoise_dz += -8.0f * t23 * t3 * z3 * dot(gx3, gy3, gz3, x3, y3, z3) + t43 * gz3;
	 */
	const float temp0 = t20 * t0 * (gx0 * x0 + gy0 * y0 + gz0 * z0);
	float dnoise_dx = temp0 * x0;
	float dnoise_dy = temp0 * y0;
	float dnoise_dz = temp0 * z0;
	const float temp1 = t21 * t1 * (gx1 * x1 + gy1 * y1 + gz1 * z1);
	dnoise_dx += temp1 * x1;
	dnoise_dy += temp1 * y1;
	dnoise_dz += temp1 * z1;
	const float temp2 = t22 * t2 * (gx2 * x2 + gy2 * y2 + gz2 * z2);
	dnoise_dx += temp2 * x2;
	dnoise_dy += temp2 * y2;
	dnoise_dz += temp2 * z2;
	const float temp3 = t23 * t3 * (gx3 * x3 + gy3 * y3 + gz3 * z3);
	dnoise_dx += temp3 * x3;
	dnoise_dy += temp3 * y3;
	dnoise_dz += temp3 * z3;
	dnoise_dx *= -8.0f;
	dnoise_dy *= -8.0f;
	dnoise_dz *= -8.0f;
	dnoise_dx += t40 * gx0 + t41 * gx1 + t42 * gx2 + t43 * gx3;
	dnoise_dy += t40 * gy0 + t41 * gy1 + t42 * gy2 + t43 * gy3;
	dnoise_dz += t40 * gz0 + t41 * gz1 + t42 * gz2 + t43 * gz3;
	dnoise_dx *= 28.0f; /* Scale derivative to match the noise scaling */
	dnoise_dy *= 28.0f;
	dnoise_dz *= 28.0f;

	return (float4)(noise, dnoise_dx, dnoise_dy, dnoise_dz);
}

float _worleyNoise2(float2 v) {
	const float2 p = floor(v);
	float2 f;
	fract(v, &f);

	float res = 8.0f;
	for (int j = -1; j <= 1; ++j) {
		for (int i = -1; i <= 1; ++i) {
			const float2 b = (float2)(i, j);
			const float2 r = b - f + (_noise2(p + b) * 0.5f + 0.5f);
			const float d = dot(r, r);
			res = min(res, d);
		}
	}
	return sqrt(res) * 2.0f - 1.0f;
}

float _worleyNoise3(float3 v) {
	const float3 p = floor(v);
	float3 f;
	fract(v, &f);

	float res = 8.0f;
	for (int k = -1; k <= 1; ++k) {
		for (int j = -1; j <= 1; ++j) {
			for (int i = -1; i <= 1; ++i) {
				const float3 b = (float3)(i, j, k);
				const float3 r = b - f + (_noise3(p + b) * 0.5f + 0.5f);
				const float d = dot(r, r);
				res = min(res, d);
			}
		}
	}
	return sqrt(res) * 2.0f - 1.0f;
}

float _worleyNoise2Falloff(float2 v, float falloff) {
	const float2 p = floor(v);
	float2 f;
	fract(v, &f);

	float res = 0.0f;
	for (int j = -1; j <= 1; ++j) {
		for (int i = -1; i <= 1; ++i) {
			const float2 b = (float2)(i, j);
			const float2 r = b - f + (_noise2(p + b) * 0.5f + 0.5f);
			const float d = sqrt(dot(r, r));
			res += exp(-falloff * d);
		}
	}
	return -(1.0f / falloff) * log(res);
}

float _worleyNoise3Falloff(float3 v, float falloff) {
	const float3 p = floor(v);
	float3 f;
	fract(v, &f);

	float res = 0.0f;
	for (int k = -1; k <= 1; ++k) {
		for (int j = -1; j <= 1; ++j) {
			for (int i = -1; i <= 1; ++i) {
				const float3 b = (float3)(i, j, k);
				const float3 r = b - f + (_noise3(p + b) * 0.5f + 0.5f);
				const float d = sqrt(dot(r, r));
				res += exp(-falloff * d);
			}
		}
	}
	return -(1.0f / falloff) * log(res);
}

float _flowNoise2(float2 v, float angle) {
	/* Sine and cosine for the gradient rotation angle */
	const float sin_t = sin(angle);
	const float cos_t = cos(angle);

	/* Skew the input space to determine which simplex cell we're in */
	const float s = (v.x + v.y) * F2; /* Hairy factor for 2D */
	const float xs = v.x + s;
	const float ys = v.y + s;
	const int i = floor(xs);
	const int j = floor(ys);

	const float t = (float) (i + j) * G2;
	const float X0 = i - t; /* Unskew the cell origin back to (x,y) space */
	const float Y0 = j - t;
	const float x0 = v.x - X0; /* The x,y distances from the cell origin */
	const float y0 = v.y - Y0;

	/* For the 2D case, the simplex shape is an equilateral triangle.
	 * Determine which simplex we are in. */
	int i1, j1; /* Offsets for second (middle) corner of simplex in (i,j) coords */
	if (x0 > y0) {
		i1 = 1;
		j1 = 0;
	} /* lower triangle, XY order: (0,0)->(1,0)->(1,1) */
	else {
		i1 = 0;
		j1 = 1;
	} /* upper triangle, YX order: (0,0)->(0,1)->(1,1) */

	/* A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
	 * a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
	 * c = (3-sqrt(3))/6   */
	const float x1 = x0 - i1 + G2; /* Offsets for middle corner in (x,y) unskewed coords */
	const float y1 = y0 - j1 + G2;
	const float x2 = x0 - 1.0f + 2.0f * G2; /* Offsets for last corner in (x,y) unskewed coords */
	const float y2 = y0 - 1.0f + 2.0f * G2;

	/* Wrap the integer indices at 256, to avoid indexing perm[] out of bounds */
	const int ii = i & 0xff;
	const int jj = j & 0xff;

	float n0, n1, n2; /* Noise contributions from the three simplex corners */
	float gx0, gy0, gx1, gy1, gx2, gy2; /* Gradients at simplex corners */

	/* Calculate the contribution from the three corners */
	float t0 = 0.5f - x0 * x0 - y0 * y0;
	float t20, t40;
	if (t0 < 0.0f)
		t40 = t20 = t0 = n0 = gx0 = gy0 = 0.0f; /* No influence */
	else {
		_gradrot2(perm[ii + perm[jj]], sin_t, cos_t, &gx0, &gy0);
		t20 = t0 * t0;
		t40 = t20 * t20;
		n0 = t40 * _graddotp2(gx0, gy0, x0, y0);
	}

	float t1 = 0.5f - x1 * x1 - y1 * y1;
	float t21, t41;
	if (t1 < 0.0f)
		t21 = t41 = t1 = n1 = gx1 = gy1 = 0.0f; /* No influence */
	else {
		_gradrot2(perm[ii + i1 + perm[jj + j1]], sin_t, cos_t, &gx1, &gy1);
		t21 = t1 * t1;
		t41 = t21 * t21;
		n1 = t41 * _graddotp2(gx1, gy1, x1, y1);
	}

	float t2 = 0.5f - x2 * x2 - y2 * y2;
	float t22, t42;
	if (t2 < 0.0f)
		t42 = t22 = t2 = n2 = gx2 = gy2 = 0.0f; /* No influence */
	else {
		_gradrot2(perm[ii + 1 + perm[jj + 1]], sin_t, cos_t, &gx2, &gy2);
		t22 = t2 * t2;
		t42 = t22 * t22;
		n2 = t42 * _graddotp2(gx2, gy2, x2, y2);
	}

	/* Add contributions from each corner to get the final noise value.
	 * The result is scaled to return values in the interval [-1,1]. */
	return 40.0f * (n0 + n1 + n2);
}

float _flowNoise3(float3 v, float angle) {
	float n0, n1, n2, n3; /* Noise contributions from the four simplex corners */
	float gx0, gy0, gz0, gx1, gy1, gz1; /* Gradients at simplex corners */
	float gx2, gy2, gz2, gx3, gy3, gz3;
	float sin_t, cos_t; /* Sine and cosine for the gradient rotation angle */
	sin_t = sin(angle);
	cos_t = cos(angle);

	/* Skew the input space to determine which simplex cell we're in */
	const float s = (v.x + v.y + v.z) * F3; /* Very nice and simple skew factor for 3D */
	const float xs = v.x + s;
	const float ys = v.y + s;
	const float zs = v.z + s;
	const int i = floor(xs);
	const int j = floor(ys);
	const int k = floor(zs);

	const float t = (float) (i + j + k) * G3;
	const float X0 = i - t; /* Unskew the cell origin back to (x,y,z) space */
	const float Y0 = j - t;
	const float Z0 = k - t;
	const float x0 = v.x - X0; /* The x,y,z distances from the cell origin */
	const float y0 = v.y - Y0;
	const float z0 = v.z - Z0;

	/* For the 3D case, the simplex shape is a slightly irregular tetrahedron.
	 * Determine which simplex we are in. */
	int i1, j1, k1; /* Offsets for second corner of simplex in (i,j,k) coords */
	int i2, j2, k2; /* Offsets for third corner of simplex in (i,j,k) coords */

	/* TODO: This code would benefit from a backport from the GLSL version! */
	if (x0 >= y0) {
		if (y0 >= z0) {
			i1 = 1;
			j1 = 0;
			k1 = 0;
			i2 = 1;
			j2 = 1;
			k2 = 0;
		} /* X Y Z order */
		else if (x0 >= z0) {
			i1 = 1;
			j1 = 0;
			k1 = 0;
			i2 = 1;
			j2 = 0;
			k2 = 1;
		} /* X Z Y order */
		else {
			i1 = 0;
			j1 = 0;
			k1 = 1;
			i2 = 1;
			j2 = 0;
			k2 = 1;
		} /* Z X Y order */
	} else { // x0<y0
		if (y0 < z0) {
			i1 = 0;
			j1 = 0;
			k1 = 1;
			i2 = 0;
			j2 = 1;
			k2 = 1;
		} /* Z Y X order */
		else if (x0 < z0) {
			i1 = 0;
			j1 = 1;
			k1 = 0;
			i2 = 0;
			j2 = 1;
			k2 = 1;
		} /* Y Z X order */
		else {
			i1 = 0;
			j1 = 1;
			k1 = 0;
			i2 = 1;
			j2 = 1;
			k2 = 0;
		} /* Y X Z order */
	}

	/* A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
	 * a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
	 * a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
	 * c = 1/6.   */

	const float x1 = x0 - i1 + G3; /* Offsets for second corner in (x,y,z) coords */
	const float y1 = y0 - j1 + G3;
	const float z1 = z0 - k1 + G3;
	const float x2 = x0 - i2 + 2.0f * G3; /* Offsets for third corner in (x,y,z) coords */
	const float y2 = y0 - j2 + 2.0f * G3;
	const float z2 = z0 - k2 + 2.0f * G3;
	const float x3 = x0 - 1.0f + 3.0f * G3; /* Offsets for last corner in (x,y,z) coords */
	const float y3 = y0 - 1.0f + 3.0f * G3;
	const float z3 = z0 - 1.0f + 3.0f * G3;

	/* Wrap the integer indices at 256, to avoid indexing perm[] out of bounds */
	const int ii = i & 0xff;
	const int jj = j & 0xff;
	const int kk = k & 0xff;

	/* Calculate the contribution from the four corners */
	float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
	float t20, t40;
	if (t0 < 0.0f)
		n0 = t0 = t20 = t40 = gx0 = gy0 = gz0 = 0.0f;
	else {
		_gradrot3(perm[ii + perm[jj + perm[kk]]], sin_t, cos_t, &gx0, &gy0, &gz0);
		t20 = t0 * t0;
		t40 = t20 * t20;
		n0 = t40 * _graddotp3(gx0, gy0, gz0, x0, y0, z0);
	}

	float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
	float t21, t41;
	if (t1 < 0.0f)
		n1 = t1 = t21 = t41 = gx1 = gy1 = gz1 = 0.0f;
	else {
		_gradrot3(perm[ii + i1 + perm[jj + j1 + perm[kk + k1]]], sin_t, cos_t, &gx1, &gy1, &gz1);
		t21 = t1 * t1;
		t41 = t21 * t21;
		n1 = t41 * _graddotp3(gx1, gy1, gz1, x1, y1, z1);
	}

	float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
	float t22, t42;
	if (t2 < 0.0f)
		n2 = t2 = t22 = t42 = gx2 = gy2 = gz2 = 0.0f;
	else {
		_gradrot3(perm[ii + i2 + perm[jj + j2 + perm[kk + k2]]], sin_t, cos_t, &gx2, &gy2, &gz2);
		t22 = t2 * t2;
		t42 = t22 * t22;
		n2 = t42 * _graddotp3(gx2, gy2, gz2, x2, y2, z2);
	}

	float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
	float t23, t43;
	if (t3 < 0.0f)
		n3 = t3 = t23 = t43 = gx3 = gy3 = gz3 = 0.0f;
	else {
		_gradrot3(perm[ii + 1 + perm[jj + 1 + perm[kk + 1]]], sin_t, cos_t, &gx3, &gy3, &gz3);
		t23 = t3 * t3;
		t43 = t23 * t23;
		n3 = t43 * _graddotp3(gx3, gy3, gz3, x3, y3, z3);
	}

	/*  Add contributions from each corner to get the final noise value.
	 * The result is scaled to return values in the range [-1,1] */
	return 28.0f * (n0 + n1 + n2 + n3);
}

float3 _dFlowNoise2(float2 v, float angle) {
	float n0, n1, n2; /* Noise contributions from the three simplex corners */
	float gx0, gy0, gx1, gy1, gx2, gy2; /* Gradients at simplex corners */
	float sin_t, cos_t; /* Sine and cosine for the gradient rotation angle */
	sin_t = sin(angle);
	cos_t = cos(angle);

	/* Skew the input space to determine which simplex cell we're in */
	const float s = (v.x + v.y) * F2; /* Hairy factor for 2D */
	const float xs = v.x + s;
	const float ys = v.y + s;
	const int i = floor(xs);
	const int j = floor(ys);

	const float t = (float) (i + j) * G2;
	const float X0 = i - t; /* Unskew the cell origin back to (x,y) space */
	const float Y0 = j - t;
	const float x0 = v.x - X0; /* The x,y distances from the cell origin */
	const float y0 = v.y - Y0;

	/* For the 2D case, the simplex shape is an equilateral triangle.
	 * Determine which simplex we are in. */
	int i1, j1; /* Offsets for second (middle) corner of simplex in (i,j) coords */
	if (x0 > y0) {
		i1 = 1;
		j1 = 0;
	} /* lower triangle, XY order: (0,0)->(1,0)->(1,1) */
	else {
		i1 = 0;
		j1 = 1;
	} /* upper triangle, YX order: (0,0)->(0,1)->(1,1) */

	/* A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
	 * a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
	 * c = (3-sqrt(3))/6   */
	const float x1 = x0 - i1 + G2; /* Offsets for middle corner in (x,y) unskewed coords */
	const float y1 = y0 - j1 + G2;
	const float x2 = x0 - 1.0f + 2.0f * G2; /* Offsets for last corner in (x,y) unskewed coords */
	const float y2 = y0 - 1.0f + 2.0f * G2;

	/* Wrap the integer indices at 256, to avoid indexing perm[] out of bounds */
	const int ii = i & 0xff;
	const int jj = j & 0xff;

	/* Calculate the contribution from the three corners */
	float t0 = 0.5f - x0 * x0 - y0 * y0;
	float t20, t40;
	if (t0 < 0.0f)
		t40 = t20 = t0 = n0 = gx0 = gy0 = 0.0f; /* No influence */
	else {
		_gradrot2(perm[ii + perm[jj]], sin_t, cos_t, &gx0, &gy0);
		t20 = t0 * t0;
		t40 = t20 * t20;
		n0 = t40 * _graddotp2(gx0, gy0, x0, y0);
	}

	float t1 = 0.5f - x1 * x1 - y1 * y1;
	float t21, t41;
	if (t1 < 0.0f)
		t21 = t41 = t1 = n1 = gx1 = gy1 = 0.0f; /* No influence */
	else {
		_gradrot2(perm[ii + i1 + perm[jj + j1]], sin_t, cos_t, &gx1, &gy1);
		t21 = t1 * t1;
		t41 = t21 * t21;
		n1 = t41 * _graddotp2(gx1, gy1, x1, y1);
	}

	float t2 = 0.5f - x2 * x2 - y2 * y2;
	float t22, t42;
	if (t2 < 0.0f)
		t42 = t22 = t2 = n2 = gx2 = gy2 = 0.0f; /* No influence */
	else {
		_gradrot2(perm[ii + 1 + perm[jj + 1]], sin_t, cos_t, &gx2, &gy2);
		t22 = t2 * t2;
		t42 = t22 * t22;
		n2 = t42 * _graddotp2(gx2, gy2, x2, y2);
	}

	/* Add contributions from each corner to get the final noise value.
	 * The result is scaled to return values in the interval [-1,1]. */
	float noise = 40.0f * (n0 + n1 + n2);

	/* Compute derivative, if requested by supplying non-null pointers
	 * for the last two arguments */
	float dnoise_dx, dnoise_dy;

	/*  A straight, unoptimised calculation would be like:
	 *    *dnoise_dx = -8.0f * t20 * t0 * x0 * graddotp2(gx0, gy0, x0, y0) + t40 * gx0;
	 *    *dnoise_dy = -8.0f * t20 * t0 * y0 * graddotp2(gx0, gy0, x0, y0) + t40 * gy0;
	 *    *dnoise_dx += -8.0f * t21 * t1 * x1 * graddotp2(gx1, gy1, x1, y1) + t41 * gx1;
	 *    *dnoise_dy += -8.0f * t21 * t1 * y1 * graddotp2(gx1, gy1, x1, y1) + t41 * gy1;
	 *    *dnoise_dx += -8.0f * t22 * t2 * x2 * graddotp2(gx2, gy2, x2, y2) + t42 * gx2;
	 *    *dnoise_dy += -8.0f * t22 * t2 * y2 * graddotp2(gx2, gy2, x2, y2) + t42 * gy2;
	 */
	const float temp0 = t20 * t0 * _graddotp2(gx0, gy0, x0, y0);
	dnoise_dx = temp0 * x0;
	dnoise_dy = temp0 * y0;
	const float temp1 = t21 * t1 * _graddotp2(gx1, gy1, x1, y1);
	dnoise_dx += temp1 * x1;
	dnoise_dy += temp1 * y1;
	const float temp2 = t22 * t2 * _graddotp2(gx2, gy2, x2, y2);
	dnoise_dx += temp2 * x2;
	dnoise_dy += temp2 * y2;
	dnoise_dx *= -8.0f;
	dnoise_dy *= -8.0f;
	/* This corrects a bug in the original implementation */
	dnoise_dx += t40 * gx0 + t41 * gx1 + t42 * gx2;
	dnoise_dy += t40 * gy0 + t41 * gy1 + t42 * gy2;
	dnoise_dx *= 40.0f; /* Scale derivative to match the noise scaling */
	dnoise_dy *= 40.0f;

	return (float3)(noise, dnoise_dx, dnoise_dy);
}

float4 _dFlowNoise3(float3 v, float angle) {
	float n0, n1, n2, n3; /* Noise contributions from the four simplex corners */
	float noise; /* Return value */
	float gx0, gy0, gz0, gx1, gy1, gz1; /* Gradients at simplex corners */
	float gx2, gy2, gz2, gx3, gy3, gz3;
	float sin_t, cos_t; /* Sine and cosine for the gradient rotation angle */
	sin_t = sin(angle);
	cos_t = cos(angle);

	/* Skew the input space to determine which simplex cell we're in */
	const float s = (v.x + v.y + v.z) * F3; /* Very nice and simple skew factor for 3D */
	const float xs = v.x + s;
	const float ys = v.y + s;
	const float zs = v.z + s;
	const int i = floor(xs);
	const int j = floor(ys);
	const int k = floor(zs);

	const float t = (float) (i + j + k) * G3;
	const float X0 = i - t; /* Unskew the cell origin back to (x,y,z) space */
	const float Y0 = j - t;
	const float Z0 = k - t;
	const float x0 = v.x - X0; /* The x,y,z distances from the cell origin */
	const float y0 = v.y - Y0;
	const float z0 = v.z - Z0;

	/* For the 3D case, the simplex shape is a slightly irregular tetrahedron.
	 * Determine which simplex we are in. */
	int i1, j1, k1; /* Offsets for second corner of simplex in (i,j,k) coords */
	int i2, j2, k2; /* Offsets for third corner of simplex in (i,j,k) coords */

	/* TODO: This code would benefit from a backport from the GLSL version! */
	if (x0 >= y0) {
		if (y0 >= z0) {
			i1 = 1;
			j1 = 0;
			k1 = 0;
			i2 = 1;
			j2 = 1;
			k2 = 0;
		} /* X Y Z order */
		else if (x0 >= z0) {
			i1 = 1;
			j1 = 0;
			k1 = 0;
			i2 = 1;
			j2 = 0;
			k2 = 1;
		} /* X Z Y order */
		else {
			i1 = 0;
			j1 = 0;
			k1 = 1;
			i2 = 1;
			j2 = 0;
			k2 = 1;
		} /* Z X Y order */
	} else { // x0<y0
		if (y0 < z0) {
			i1 = 0;
			j1 = 0;
			k1 = 1;
			i2 = 0;
			j2 = 1;
			k2 = 1;
		} /* Z Y X order */
		else if (x0 < z0) {
			i1 = 0;
			j1 = 1;
			k1 = 0;
			i2 = 0;
			j2 = 1;
			k2 = 1;
		} /* Y Z X order */
		else {
			i1 = 0;
			j1 = 1;
			k1 = 0;
			i2 = 1;
			j2 = 1;
			k2 = 0;
		} /* Y X Z order */
	}

	/* A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
	 * a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
	 * a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
	 * c = 1/6.   */

	float x1 = x0 - i1 + G3; /* Offsets for second corner in (x,y,z) coords */
	float y1 = y0 - j1 + G3;
	float z1 = z0 - k1 + G3;
	float x2 = x0 - i2 + 2.0f * G3; /* Offsets for third corner in (x,y,z) coords */
	float y2 = y0 - j2 + 2.0f * G3;
	float z2 = z0 - k2 + 2.0f * G3;
	float x3 = x0 - 1.0f + 3.0f * G3; /* Offsets for last corner in (x,y,z) coords */
	float y3 = y0 - 1.0f + 3.0f * G3;
	float z3 = z0 - 1.0f + 3.0f * G3;

	/* Wrap the integer indices at 256, to avoid indexing perm[] out of bounds */
	int ii = i & 0xff;
	int jj = j & 0xff;
	int kk = k & 0xff;

	/* Calculate the contribution from the four corners */
	float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
	float t20, t40;
	if (t0 < 0.0f)
		n0 = t0 = t20 = t40 = gx0 = gy0 = gz0 = 0.0f;
	else {
		_gradrot3(perm[ii + perm[jj + perm[kk]]], sin_t, cos_t, &gx0, &gy0, &gz0);
		t20 = t0 * t0;
		t40 = t20 * t20;
		n0 = t40 * _graddotp3(gx0, gy0, gz0, x0, y0, z0);
	}

	float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
	float t21, t41;
	if (t1 < 0.0f)
		n1 = t1 = t21 = t41 = gx1 = gy1 = gz1 = 0.0f;
	else {
		_gradrot3(perm[ii + i1 + perm[jj + j1 + perm[kk + k1]]], sin_t, cos_t, &gx1, &gy1, &gz1);
		t21 = t1 * t1;
		t41 = t21 * t21;
		n1 = t41 * _graddotp3(gx1, gy1, gz1, x1, y1, z1);
	}

	float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
	float t22, t42;
	if (t2 < 0.0f)
		n2 = t2 = t22 = t42 = gx2 = gy2 = gz2 = 0.0f;
	else {
		_gradrot3(perm[ii + i2 + perm[jj + j2 + perm[kk + k2]]], sin_t, cos_t, &gx2, &gy2, &gz2);
		t22 = t2 * t2;
		t42 = t22 * t22;
		n2 = t42 * _graddotp3(gx2, gy2, gz2, x2, y2, z2);
	}

	float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
	float t23, t43;
	if (t3 < 0.0f)
		n3 = t3 = t23 = t43 = gx3 = gy3 = gz3 = 0.0f;
	else {
		_gradrot3(perm[ii + 1 + perm[jj + 1 + perm[kk + 1]]], sin_t, cos_t, &gx3, &gy3, &gz3);
		t23 = t3 * t3;
		t43 = t23 * t23;
		n3 = t43 * _graddotp3(gx3, gy3, gz3, x3, y3, z3);
	}

	/*  Add contributions from each corner to get the final noise value.
	 * The result is scaled to return values in the range [-1,1] */
	noise = 28.0f * (n0 + n1 + n2 + n3);

	/* Compute derivative, if requested by supplying non-null pointers
	 * for the last three arguments */
	float dnoise_dx, dnoise_dy, dnoise_dz;

	/*  A straight, unoptimised calculation would be like:
	 *     *dnoise_dx = -8.0f * t20 * t0 * x0 * graddotp3(gx0, gy0, gz0, x0, y0, z0) + t40 * gx0;
	 *    *dnoise_dy = -8.0f * t20 * t0 * y0 * graddotp3(gx0, gy0, gz0, x0, y0, z0) + t40 * gy0;
	 *    *dnoise_dz = -8.0f * t20 * t0 * z0 * graddotp3(gx0, gy0, gz0, x0, y0, z0) + t40 * gz0;
	 *    *dnoise_dx += -8.0f * t21 * t1 * x1 * graddotp3(gx1, gy1, gz1, x1, y1, z1) + t41 * gx1;
	 *    *dnoise_dy += -8.0f * t21 * t1 * y1 * graddotp3(gx1, gy1, gz1, x1, y1, z1) + t41 * gy1;
	 *    *dnoise_dz += -8.0f * t21 * t1 * z1 * graddotp3(gx1, gy1, gz1, x1, y1, z1) + t41 * gz1;
	 *    *dnoise_dx += -8.0f * t22 * t2 * x2 * graddotp3(gx2, gy2, gz2, x2, y2, z2) + t42 * gx2;
	 *    *dnoise_dy += -8.0f * t22 * t2 * y2 * graddotp3(gx2, gy2, gz2, x2, y2, z2) + t42 * gy2;
	 *    *dnoise_dz += -8.0f * t22 * t2 * z2 * graddotp3(gx2, gy2, gz2, x2, y2, z2) + t42 * gz2;
	 *    *dnoise_dx += -8.0f * t23 * t3 * x3 * graddotp3(gx3, gy3, gz3, x3, y3, z3) + t43 * gx3;
	 *    *dnoise_dy += -8.0f * t23 * t3 * y3 * graddotp3(gx3, gy3, gz3, x3, y3, z3) + t43 * gy3;
	 *    *dnoise_dz += -8.0f * t23 * t3 * z3 * graddotp3(gx3, gy3, gz3, x3, y3, z3) + t43 * gz3;
	 */
	const float temp0 = t20 * t0 * _graddotp3(gx0, gy0, gz0, x0, y0, z0);
	dnoise_dx = temp0 * x0;
	dnoise_dy = temp0 * y0;
	dnoise_dz = temp0 * z0;
	const float temp1 = t21 * t1 * _graddotp3(gx1, gy1, gz1, x1, y1, z1);
	dnoise_dx += temp1 * x1;
	dnoise_dy += temp1 * y1;
	dnoise_dz += temp1 * z1;
	const float temp2 = t22 * t2 * _graddotp3(gx2, gy2, gz2, x2, y2, z2);
	dnoise_dx += temp2 * x2;
	dnoise_dy += temp2 * y2;
	dnoise_dz += temp2 * z2;
	const float temp3 = t23 * t3 * _graddotp3(gx3, gy3, gz3, x3, y3, z3);
	dnoise_dx += temp3 * x3;
	dnoise_dy += temp3 * y3;
	dnoise_dz += temp3 * z3;
	dnoise_dx *= -8.0f;
	dnoise_dy *= -8.0f;
	dnoise_dz *= -8.0f;
	/* This corrects a bug in the original implementation */
	dnoise_dx += t40 * gx0 + t41 * gx1 + t42 * gx2 + t43 * gx3;
	dnoise_dy += t40 * gy0 + t41 * gy1 + t42 * gy2 + t43 * gy3;
	dnoise_dz += t40 * gz0 + t41 * gz1 + t42 * gz2 + t43 * gz3;
	dnoise_dx *= 28.0f; /* Scale derivative to match the noise scaling */
	dnoise_dy *= 28.0f;
	dnoise_dz *= 28.0f;

	return (float4)(noise, dnoise_dx, dnoise_dy, dnoise_dz);
}

float _fBm1(float x, uchar octaves, float lacunarity, float gain) {
	float sum = 0.0f;
	float freq = 1.0f;
	float amp = 0.5f;

	for (int i = 0; i < octaves; ++i) {
		const float n = _noise1(x * freq);
		sum += n * amp;
		freq *= lacunarity;
		amp *= gain;
	}

	return sum;
}

float _fBm2(float2 v, uchar octaves, float lacunarity, float gain) {
	float sum = 0.0f;
	float freq = 1.0f;
	float amp = 0.5f;

	for (int i = 0; i < octaves; ++i) {
		const float n = _noise2(v * freq);
		sum += n * amp;
		freq *= lacunarity;
		amp *= gain;
	}

	return sum;
}

float _fBm3(float3 v, uchar octaves, float lacunarity, float gain) {
	float sum = 0.0f;
	float freq = 1.0f;
	float amp = 0.5f;

	for (int i = 0; i < octaves; ++i) {
		const float n = _noise3(v * freq);
		sum += n * amp;
		freq *= lacunarity;
		amp *= gain;
	}

	return sum;
}

float _fBm4(float4 v, uchar octaves, float lacunarity, float gain) {
	float sum = 0.0f;
	float freq = 1.0f;
	float amp = 0.5f;

	for (int i = 0; i < octaves; ++i) {
		const float n = _noise4(v * freq);
		sum += n * amp;
		freq *= lacunarity;
		amp *= gain;
	}

	return sum;
}

float _worleyfBm2(float2 v, uchar octaves, float lacunarity, float gain) {
	float sum = 0.0f;
	float freq = 1.0f;
	float amp = 0.5f;

	for (int i = 0; i < octaves; ++i) {
		const float n = _worleyNoise2(v * freq);
		sum += n * amp;
		freq *= lacunarity;
		amp *= gain;
	}

	return sum;
}

float _worleyfBm3(float3 v, uchar octaves, float lacunarity, float gain) {
	float sum = 0.0f;
	float freq = 1.0f;
	float amp = 0.5f;

	for (int i = 0; i < octaves; ++i) {
		const float n = _worleyNoise3(v * freq);
		sum += n * amp;
		freq *= lacunarity;
		amp *= gain;
	}

	return sum;
}

float _worleyfBm2Falloff(float2 v, float falloff, uchar octaves, float lacunarity, float gain) {
	float sum = 0.0f;
	float freq = 1.0f;
	float amp = 0.5f;
	for (int i = 0; i < octaves; ++i) {
		const float n = _worleyNoise2Falloff(v * freq, falloff);
		sum += n * amp;
		freq *= lacunarity;
		amp *= gain;
	}
	return sum;
}

float _worleyfBm3Falloff(float3 v, float falloff, uchar octaves, float lacunarity, float gain) {
	float s = 0.0f;
	float freq = 1.0f;
	float amp = 0.5f;
	for (int i = 0; i < octaves; ++i) {
		const float n = _worleyNoise3Falloff(v * freq, falloff);
		s += n * amp;
		freq *= lacunarity;
		amp *= gain;
	}
	return s;
}

float2 _dfBm1(float x, uchar octaves, float lacunarity, float gain) {
	float2 s = (float2)(0.0f, 0.0f);
	float freq = 1.0f;
	float amp = 0.5f;
	for (int i = 0; i < octaves; ++i) {
		const float2 n = _dnoise1(x * freq);
		s += n * amp;
		freq *= lacunarity;
		amp *= gain;
	}
	return s;
}

float3 _dfBm2(float2 v, uchar octaves, float lacunarity, float gain) {
	float3 s = (float3)(0.0f, 0.0f, 0.0f);
	float freq = 1.0f;
	float amp = 0.5f;
	for (int i = 0; i < octaves; ++i) {
		const float3 n = _dnoise2(v * freq);
		s += n * amp;
		freq *= lacunarity;
		amp *= gain;
	}
	return s;
}

float4 _dfBm3(float3 v, uchar octaves, float lacunarity, float gain) {
	float4 s = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
	float freq = 1.0f;
	float amp = 0.5f;
	for (int i = 0; i < octaves; ++i) {
		const float4 n = _dnoise3(v * freq);
		s += n * amp;
		freq *= lacunarity;
		amp *= gain;
	}
	return s;
}

float _ridgedNoise1(float x) {
	return (1.0f - fabs(_noise1(x))) * 2.0f - 1.0f;
}

float _ridgedNoise2(float2 v) {
	return (1.0f - fabs(_noise2(v))) * 2.0f - 1.0f;
}

float _ridgedNoise3(float3 v) {
	return (1.0f - fabs(_noise3(v))) * 2.0f - 1.0f;
}

float _ridgedNoise4(float4 v) {
	return (1.0f - fabs(_noise4(v))) * 2.0f - 1.0f;
}

float _ridge(float h, float offset) {
	h = offset - fabs(h);
	return h * h;
}

float _ridgedMF1(float x, float ridgeOffset, uchar octaves, float lacunarity, float gain) {
	float sum = 0.0f;
	float freq = 1.0f;
	float amp = 0.5f;
	float prev = 1.0f;

	for (int i = 0; i < octaves; ++i) {
		const float n = _ridge(_noise1(x * freq), ridgeOffset);
		sum += n * amp * prev;
		prev = n;
		freq *= lacunarity;
		amp *= gain;
	}
	return sum * 2.0f - 0.5f;
}

float _ridgedMF2(float2 v, float ridgeOffset, uchar octaves, float lacunarity, float gain) {
	float sum = 0.0f;
	float freq = 1.0f;
	float amp = 0.5f;
	float prev = 1.0f;

	for (int i = 0; i < octaves; ++i) {
		const float n = _ridge(_noise2(v * freq), ridgeOffset);
		sum += n * amp * prev;
		prev = n;
		freq *= lacunarity;
		amp *= gain;
	}
	return sum * 2.0f - 0.5f;
}

float _ridgedMF3(float3 v, float ridgeOffset, uchar octaves, float lacunarity, float gain) {
	float sum = 0.0f;
	float freq = 1.0f;
	float amp = 0.5f;
	float prev = 1.0f;

	for (int i = 0; i < octaves; ++i) {
		const float n = _ridge(_noise3(v * freq), ridgeOffset);
		sum += n * amp * prev;
		prev = n;
		freq *= lacunarity;
		amp *= gain;
	}
	return sum * 2.0f - 0.5f;
}

float _ridgedMF4(float4 v, float ridgeOffset, uchar octaves, float lacunarity, float gain) {
	float sum = 0.0f;
	float freq = 1.0f;
	float amp = 0.5f;
	float prev = 1.0f;

	for (int i = 0; i < octaves; ++i) {
		const float n = _ridge(_noise4(v * freq), ridgeOffset);
		sum += n * amp * prev;
		prev = n;
		freq *= lacunarity;
		amp *= gain;
	}
	return sum * 2.0f - 0.5f;
}

float _iqfBm2(float2 v, uchar octaves, float lacunarity, float gain) {
	float sum = 0.0f;
	float amp = 0.5f;
	float2 delta = (float2)(0.0f, 0.0f);
	float freq = 1.0f;
	for (int i = 0; i < octaves; ++i) {
		const float3 d = _dnoise2(v * freq);
		delta.x += d.y;
		delta.y += d.y;
		sum += amp * d.x / (1.0f + dot(delta, delta));
		freq *= lacunarity;
		amp *= gain;
	}

	return sum;
}

float _iqfBm3(float3 v, uchar octaves, float lacunarity, float gain) {
	float sum = 0.0f;
	float amp = 0.5f;
	float3 delta = (float3)(0.0f, 0.0f, 0.0f);
	float freq = 1.0f;
	for (int i = 0; i < octaves; ++i) {
		const float4 d = _dnoise3(v * freq);
		delta.x += d.y;
		delta.y += d.y;
		delta.z += d.z;
		sum += amp * d.x / (1.0f + dot(delta, delta));
		freq *= lacunarity;
		amp *= gain;
	}

	return sum;
}

float2 _curlNoise2(float2 v) {
	const float3 derivative = _dnoise2(v);
	return (float2)(derivative.z, -derivative.y);
}

float2 _curlNoise2Time(float2 v, float t) {
	const float3 derivative = _dFlowNoise2(v, t);
	return (float2)(derivative.z, -derivative.y);
}

float2 _curlNoise2Param(float2 v, uchar octaves, float lacunarity, float gain) {
	const float3 derivative = _dfBm2(v, octaves, lacunarity, gain);
	return (float2)(derivative.z, -derivative.y);
}

float3 _curlNoise3(float3 v) {
	const float4 derivX = _dnoise3(v);
	const float4 derivY = _dnoise3(v + (float3)(123.456f, 789.012f, 345.678f));
	const float4 derivZ = _dnoise3(v + (float3)(901.234f, 567.891f, 234.567f));
	return (float3)(derivZ.z - derivY.w, derivX.w - derivZ.y, derivY.y - derivX.z);
}

float3 _curlNoise3Time(float3 v, float t) {
	const float4 derivX = _dFlowNoise3(v, t);
	const float4 derivY = _dFlowNoise3(v + (float3)(123.456f, 789.012f, 345.678f), t);
	const float4 derivZ = _dFlowNoise3(v + (float3)(901.234f, 567.891f, 234.567f), t);
	return (float3)(derivZ.z - derivY.w, derivX.w - derivZ.y, derivY.y - derivX.z);
}

float3 _curlNoise3Param(float3 v, uchar octaves, float lacunarity, float gain) {
	const float4 derivX = _dfBm3(v, octaves, lacunarity, gain);
	const float4 derivY = _dfBm3(v + (float3)(123.456f, 789.012f, 345.678f), octaves, lacunarity, gain);
	const float4 derivZ = _dfBm3(v + (float3)(901.234f, 567.891f, 234.567f), octaves, lacunarity, gain);
	return (float3)(derivZ.z - derivY.w, derivX.w - derivZ.y, derivY.y - derivX.z);
}

float _norm(float noise) {
	return (clamp(noise, -1.0f, 1.0f) + 1.0f) * 0.5f;
}

__kernel void ridgedMF2(
		__global uchar4 *output,
		int components,
		float frequency,
		float amplitude,
		float ridgeOffset,
		uchar octaves,
		float lacunarity,
		float gain
)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	const int w = get_global_size(0);
	const int h = get_global_size(1);
	const int index = (x + y * w) * components;
	for (int channel = 0; channel < components; ++channel) {
		const float2 v = (float2)(x + channel, y + channel);
		const float noise = _norm(_ridgedMF2(v, ridgeOffset, octaves, lacunarity, gain));
		const uchar color = (uchar) (noise * 255.0f);
		const int channelIndex = index + channel;
		output[channelIndex] = color;
	}
}

__kernel void seamlessNoise(
		__global uchar *output,
		int size,
		int components,
		uchar octaves,
		float lacunarity,
		float gain)
{
	const float two_pi = 6.28318530717958647692528676655900576f;
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	const float d = 1.0f / (float)size;
	const float s = x * d;
	const float t = y * d;
	const float s_two_pi = s * two_pi;
	const float t_two_pi = t * two_pi;
	const float nx = cos(s_two_pi);
	const float nz = sin(s_two_pi);
	const float ny = cos(t_two_pi);
	const float nw = sin(t_two_pi);
	const int index = (x + y * size) * components;
	for (int channel = 0; channel < components; ++channel) {
		const float4 v = (float4)(nx + channel, ny + channel, nz + channel, nw + channel);
		const float noise = _norm(_fBm4(v, octaves, lacunarity, gain));
		const uchar color = (uchar) (noise * 255.0f);
		const int channelIndex = index + channel;
		output[channelIndex] = color;
	}
}
