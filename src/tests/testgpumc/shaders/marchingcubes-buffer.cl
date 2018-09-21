//#pragma OPENCL EXTENSION cl_amd_printf:enable

__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP
		| CLK_FILTER_NEAREST;

/* Morton Code Functions - Kudos to http://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/ */

// "Insert" two 0 bits after each of the 10 low bits of x
uint Part1By2(uint x) {
	x &= 0x000003ff;              // x = ---- ---- ---- ---- ---- --98 7654 3210
	x = (x ^ (x << 16)) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
	x = (x ^ (x << 8)) & 0x0300f00f; // x = ---- --98 ---- ---- 7654 ---- ---- 3210
	x = (x ^ (x << 4)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
	x = (x ^ (x << 2)) & 0x09249249; // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
	return x;
}

uint EncodeMorton3(uint x, uint y, uint z) {
	return (Part1By2(z) << 2) + (Part1By2(y) << 1) + Part1By2(x);
}

uint EncodeMorton(int4 v) {
	return EncodeMorton3(v.x, v.y, v.z);
}

// Inverse of Part1By2 - "delete" all bits not at positions divisible by 3
uint Compact1By2(uint x) {
	x &= 0x09249249;              // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
	x = (x ^ (x >> 2)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
	x = (x ^ (x >> 4)) & 0x0300f00f; // x = ---- --98 ---- ---- 7654 ---- ---- 3210
	x = (x ^ (x >> 8)) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
	x = (x ^ (x >> 16)) & 0x000003ff; // x = ---- ---- ---- ---- ---- --98 7654 3210
	return x;
}

uint DecodeMorton3X(uint code) {
	return Compact1By2(code >> 0);
}

uint DecodeMorton3Y(uint code) {
	return Compact1By2(code >> 1);
}

uint DecodeMorton3Z(uint code) {
	return Compact1By2(code >> 2);
}

__constant int4 cubeOffsets[8] = { { 0, 0, 0, 0 }, { 1, 0, 0, 0 },
		{ 0, 0, 1, 0 }, { 1, 0, 1, 0 }, { 0, 1, 0, 0 }, { 1, 1, 0, 0 }, { 0, 1,
				1, 0 }, { 1, 1, 1, 0 }, };

__kernel void constructHPLevelCharChar(
		__global uchar * readHistoPyramid,
		__global uchar * writeHistoPyramid
) {

	uint writePos = EncodeMorton3(get_global_id(0), get_global_id(1), get_global_id(2));
	uint readPos = EncodeMorton3(get_global_id(0)*2, get_global_id(1)*2, get_global_id(2)*2);
	uchar writeValue = readHistoPyramid[readPos] +
	readHistoPyramid[readPos + 1] +
	readHistoPyramid[readPos + 2] +
	readHistoPyramid[readPos + 3] +
	readHistoPyramid[readPos + 4] +
	readHistoPyramid[readPos + 5] +
	readHistoPyramid[readPos + 6] +
	readHistoPyramid[readPos + 7];

	writeHistoPyramid[writePos] = writeValue;
}

__kernel void constructHPLevelCharShort(
		__global uchar * readHistoPyramid,
		__global ushort * writeHistoPyramid
) {

	uint writePos = EncodeMorton3(get_global_id(0), get_global_id(1), get_global_id(2));
	uint readPos = EncodeMorton3(get_global_id(0)*2, get_global_id(1)*2, get_global_id(2)*2);
	ushort writeValue = readHistoPyramid[readPos] +
	readHistoPyramid[readPos + 1] +
	readHistoPyramid[readPos + 2] +
	readHistoPyramid[readPos + 3] +
	readHistoPyramid[readPos + 4] +
	readHistoPyramid[readPos + 5] +
	readHistoPyramid[readPos + 6] +
	readHistoPyramid[readPos + 7];

	writeHistoPyramid[writePos] = writeValue;
}
__kernel void constructHPLevelShortShort(
		__global ushort * readHistoPyramid,
		__global ushort * writeHistoPyramid
) {

	uint writePos = EncodeMorton3(get_global_id(0), get_global_id(1), get_global_id(2));
	uint readPos = EncodeMorton3(get_global_id(0)*2, get_global_id(1)*2, get_global_id(2)*2);
	ushort writeValue = readHistoPyramid[readPos] +
	readHistoPyramid[readPos + 1] +
	readHistoPyramid[readPos + 2] +
	readHistoPyramid[readPos + 3] +
	readHistoPyramid[readPos + 4] +
	readHistoPyramid[readPos + 5] +
	readHistoPyramid[readPos + 6] +
	readHistoPyramid[readPos + 7];

	writeHistoPyramid[writePos] = writeValue;
}

__kernel void constructHPLevelShortInt(
		__global ushort * readHistoPyramid,
		__global int * writeHistoPyramid
) {

	uint writePos = EncodeMorton3(get_global_id(0), get_global_id(1), get_global_id(2));
	uint readPos = EncodeMorton3(get_global_id(0)*2, get_global_id(1)*2, get_global_id(2)*2);
	int writeValue = readHistoPyramid[readPos] +
	readHistoPyramid[readPos + 1] +
	readHistoPyramid[readPos + 2] +
	readHistoPyramid[readPos + 3] +
	readHistoPyramid[readPos + 4] +
	readHistoPyramid[readPos + 5] +
	readHistoPyramid[readPos + 6] +
	readHistoPyramid[readPos + 7];

	writeHistoPyramid[writePos] = writeValue;
}

__kernel void constructHPLevel(
		__global int * readHistoPyramid,
		__global int * writeHistoPyramid
) {

	uint writePos = EncodeMorton3(get_global_id(0), get_global_id(1), get_global_id(2));
	uint readPos = EncodeMorton3(get_global_id(0)*2, get_global_id(1)*2, get_global_id(2)*2);
	int writeValue = readHistoPyramid[readPos] +
	readHistoPyramid[readPos + 1] +
	readHistoPyramid[readPos + 2] +
	readHistoPyramid[readPos + 3] +
	readHistoPyramid[readPos + 4] +
	readHistoPyramid[readPos + 5] +
	readHistoPyramid[readPos + 6] +
	readHistoPyramid[readPos + 7];

	writeHistoPyramid[writePos] = writeValue;
}

int4 scanHPLevelShort(int target, __global ushort * hp, int4 current) {

	int8 neighbors = {
		hp[EncodeMorton(current)],
		hp[EncodeMorton(current + cubeOffsets[1])],
		hp[EncodeMorton(current + cubeOffsets[2])],
		hp[EncodeMorton(current + cubeOffsets[3])],
		hp[EncodeMorton(current + cubeOffsets[4])],
		hp[EncodeMorton(current + cubeOffsets[5])],
		hp[EncodeMorton(current + cubeOffsets[6])],
		hp[EncodeMorton(current + cubeOffsets[7])],
	};

	int acc = current.s3 + neighbors.s0;
	int8 cmp;
	cmp.s0 = acc <= target;
	acc += neighbors.s1;
	cmp.s1 = acc <= target;
	acc += neighbors.s2;
	cmp.s2 = acc <= target;
	acc += neighbors.s3;
	cmp.s3 = acc <= target;
	acc += neighbors.s4;
	cmp.s4 = acc <= target;
	acc += neighbors.s5;
	cmp.s5 = acc <= target;
	acc += neighbors.s6;
	cmp.s6 = acc <= target;
	cmp.s7 = 0;

	current += cubeOffsets[(cmp.s0+cmp.s1+cmp.s2+cmp.s3+cmp.s4+cmp.s5+cmp.s6+cmp.s7)];
	current.s0 = current.s0*2;
	current.s1 = current.s1*2;
	current.s2 = current.s2*2;
	current.s3 = current.s3 +
	cmp.s0*neighbors.s0 +
	cmp.s1*neighbors.s1 +
	cmp.s2*neighbors.s2 +
	cmp.s3*neighbors.s3 +
	cmp.s4*neighbors.s4 +
	cmp.s5*neighbors.s5 +
	cmp.s6*neighbors.s6 +
	cmp.s7*neighbors.s7;
	return current;

}
int4 scanHPLevelChar(int target, __global uchar * hp, int4 current) {

	int8 neighbors = {
		hp[EncodeMorton(current)],
		hp[EncodeMorton(current + cubeOffsets[1])],
		hp[EncodeMorton(current + cubeOffsets[2])],
		hp[EncodeMorton(current + cubeOffsets[3])],
		hp[EncodeMorton(current + cubeOffsets[4])],
		hp[EncodeMorton(current + cubeOffsets[5])],
		hp[EncodeMorton(current + cubeOffsets[6])],
		hp[EncodeMorton(current + cubeOffsets[7])],
	};

	int acc = current.s3 + neighbors.s0;
	int8 cmp;
	cmp.s0 = acc <= target;
	acc += neighbors.s1;
	cmp.s1 = acc <= target;
	acc += neighbors.s2;
	cmp.s2 = acc <= target;
	acc += neighbors.s3;
	cmp.s3 = acc <= target;
	acc += neighbors.s4;
	cmp.s4 = acc <= target;
	acc += neighbors.s5;
	cmp.s5 = acc <= target;
	acc += neighbors.s6;
	cmp.s6 = acc <= target;
	cmp.s7 = 0;

	current += cubeOffsets[(cmp.s0+cmp.s1+cmp.s2+cmp.s3+cmp.s4+cmp.s5+cmp.s6+cmp.s7)];
	current.s0 = current.s0*2;
	current.s1 = current.s1*2;
	current.s2 = current.s2*2;
	current.s3 = current.s3 +
	cmp.s0*neighbors.s0 +
	cmp.s1*neighbors.s1 +
	cmp.s2*neighbors.s2 +
	cmp.s3*neighbors.s3 +
	cmp.s4*neighbors.s4 +
	cmp.s5*neighbors.s5 +
	cmp.s6*neighbors.s6 +
	cmp.s7*neighbors.s7;
	return current;

}

int4 scanHPLevel(int target, __global int * hp, int4 current) {

	int8 neighbors = {
		hp[EncodeMorton(current)],
		hp[EncodeMorton(current + cubeOffsets[1])],
		hp[EncodeMorton(current + cubeOffsets[2])],
		hp[EncodeMorton(current + cubeOffsets[3])],
		hp[EncodeMorton(current + cubeOffsets[4])],
		hp[EncodeMorton(current + cubeOffsets[5])],
		hp[EncodeMorton(current + cubeOffsets[6])],
		hp[EncodeMorton(current + cubeOffsets[7])],
	};

	int acc = current.s3 + neighbors.s0;
	int8 cmp;
	cmp.s0 = acc <= target;
	acc += neighbors.s1;
	cmp.s1 = acc <= target;
	acc += neighbors.s2;
	cmp.s2 = acc <= target;
	acc += neighbors.s3;
	cmp.s3 = acc <= target;
	acc += neighbors.s4;
	cmp.s4 = acc <= target;
	acc += neighbors.s5;
	cmp.s5 = acc <= target;
	acc += neighbors.s6;
	cmp.s6 = acc <= target;
	cmp.s7 = 0;

	current += cubeOffsets[(cmp.s0+cmp.s1+cmp.s2+cmp.s3+cmp.s4+cmp.s5+cmp.s6+cmp.s7)];
	current.s0 = current.s0*2;
	current.s1 = current.s1*2;
	current.s2 = current.s2*2;
	current.s3 = current.s3 +
	cmp.s0*neighbors.s0 +
	cmp.s1*neighbors.s1 +
	cmp.s2*neighbors.s2 +
	cmp.s3*neighbors.s3 +
	cmp.s4*neighbors.s4 +
	cmp.s5*neighbors.s5 +
	cmp.s6*neighbors.s6 +
	cmp.s7*neighbors.s7;
	return current;

}

__constant char offsets3[72] = {
// 0
		0, 0, 0, 1, 0, 0,
		// 1
		1, 0, 0, 1, 0, 1,
		// 2
		1, 0, 1, 0, 0, 1,
		// 3
		0, 0, 1, 0, 0, 0,
		// 4
		0, 1, 0, 1, 1, 0,
		// 5
		1, 1, 0, 1, 1, 1,
		// 6
		1, 1, 1, 0, 1, 1,
		// 7
		0, 1, 1, 0, 1, 0,
		// 8
		0, 0, 0, 0, 1, 0,
		// 9
		1, 0, 0, 1, 1, 0,
		// 10
		1, 0, 1, 1, 1, 1,
		// 11
		0, 0, 1, 0, 1, 1 };

__constant char triTable[4096] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1,
		8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 10, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 8, 3, 1, 2, 10, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, 9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, 2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1,
		-1, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 11,
		2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 9, 0, 2, 3, 11,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 11, 2, 1, 9, 11, 9, 8, 11,
		-1, -1, -1, -1, -1, -1, -1, 3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, 0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1,
		-1, 3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1, 9, 8, 10,
		10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4, 7, 8, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4, 3, 0, 7, 3, 4, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, 0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, 4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1,
		1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, 4, 7, 3,
		0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, 9, 2, 10, 9, 0, 2, 8, 4, 7,
		-1, -1, -1, -1, -1, -1, -1, 2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1,
		-1, -1, 8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 11,
		4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1, 9, 0, 1, 8, 4, 7,
		2, 3, 11, -1, -1, -1, -1, -1, -1, -1, 4, 7, 11, 9, 4, 11, 9, 11, 2, 9,
		2, 1, -1, -1, -1, -1, 3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1,
		-1, -1, 1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1, 4, 7, 8,
		9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1, 4, 7, 11, 4, 11, 9, 9,
		11, 10, -1, -1, -1, -1, -1, -1, -1, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, 9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, 0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 8,
		5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1, 1, 2, 10, 9, 5, 4,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, 0, 8, 1, 2, 10, 4, 9, 5, -1,
		-1, -1, -1, -1, -1, -1, 5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1,
		-1, -1, 2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1, 9, 5, 4, 2,
		3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 11, 2, 0, 8, 11, 4, 9,
		5, -1, -1, -1, -1, -1, -1, -1, 0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1,
		-1, -1, -1, -1, 2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1, 10,
		3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, 4, 9, 5, 0, 8, 1,
		8, 10, 1, 8, 11, 10, -1, -1, -1, -1, 5, 4, 0, 5, 0, 11, 5, 11, 10, 11,
		0, 3, -1, -1, -1, -1, 5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1,
		-1, -1, 9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 9, 3,
		0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1, 0, 7, 8, 0, 1, 7, 1, 5,
		7, -1, -1, -1, -1, -1, -1, -1, 1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, 9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1,
		10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1, 8, 0, 2, 8, 2, 5,
		8, 5, 7, 10, 5, 2, -1, -1, -1, -1, 2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1,
		-1, -1, -1, -1, -1, 7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1,
		-1, 9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1, 2, 3, 11, 0, 1,
		8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1, 11, 2, 1, 11, 1, 7, 7, 1, 5, -1,
		-1, -1, -1, -1, -1, -1, 9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1,
		-1, -1, 5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1, 11, 10, 0,
		11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1, 11, 10, 5, 7, 11, 5, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, 0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, 9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, 1, 6, 5, 2, 6,
		1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 6, 5, 1, 2, 6, 3, 0, 8,
		-1, -1, -1, -1, -1, -1, -1, 9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1,
		-1, -1, -1, 5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1, 2, 3,
		11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 11, 0, 8, 11, 2,
		0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, 0, 1, 9, 2, 3, 11, 5, 10, 6,
		-1, -1, -1, -1, -1, -1, -1, 5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1,
		-1, -1, -1, 6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1, 0,
		8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1, 3, 11, 6, 0, 3, 6,
		0, 6, 5, 0, 5, 9, -1, -1, -1, -1, 6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1,
		-1, -1, -1, -1, -1, 5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, 4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1, 1, 9, 0,
		5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, 10, 6, 5, 1, 9, 7, 1, 7,
		3, 7, 9, 4, -1, -1, -1, -1, 6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1,
		-1, -1, -1, 1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1, 8, 4, 7,
		9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1, 7, 3, 9, 7, 9, 4, 3, 2, 9, 5,
		9, 6, 2, 6, 9, -1, 3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1,
		-1, 5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1, 0, 1, 9, 4, 7,
		8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, 9, 2, 1, 9, 11, 2, 9, 4, 11, 7,
		11, 4, 5, 10, 6, -1, 8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1,
		-1, 5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1, 0, 5, 9, 0, 6,
		5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1, 6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9,
		-1, -1, -1, -1, 10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, 4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, 10, 0, 1,
		10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1, 8, 3, 1, 8, 1, 6, 8, 6,
		4, 6, 1, 10, -1, -1, -1, -1, 1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1,
		-1, -1, -1, 3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1, 0, 2, 4,
		4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 8, 3, 2, 8, 2, 4, 4, 2,
		6, -1, -1, -1, -1, -1, -1, -1, 10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1,
		-1, -1, -1, -1, 0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1,
		3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1, 6, 4, 1, 6, 1, 10,
		4, 8, 1, 2, 1, 11, 8, 11, 1, -1, 9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3,
		-1, -1, -1, -1, 8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1, 3,
		11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1, 6, 4, 8, 11, 6, 8,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 7, 10, 6, 7, 8, 10, 8, 9, 10,
		-1, -1, -1, -1, -1, -1, -1, 0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1,
		-1, -1, -1, 10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1, 10, 6,
		7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1, 1, 2, 6, 1, 6, 8, 1,
		8, 9, 8, 6, 7, -1, -1, -1, -1, 2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3,
		9, -1, 7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1, 7, 3, 2,
		6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, 3, 11, 10, 6, 8, 10,
		8, 9, 8, 6, 7, -1, -1, -1, -1, 2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9,
		10, 7, -1, 1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1, 11, 2, 1,
		11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1, 8, 9, 6, 8, 6, 7, 9, 1, 6,
		11, 6, 3, 1, 3, 6, -1, 0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, 7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1, 7, 11,
		6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 7, 6, 11, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, 0, 8, 11, 7, 6, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, 8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1,
		-1, 10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 2,
		10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, 2, 9, 0, 2, 10, 9, 6,
		11, 7, -1, -1, -1, -1, -1, -1, -1, 6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9,
		8, -1, -1, -1, -1, 7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, 7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1, 2, 7, 6, 2,
		3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1, 1, 6, 2, 1, 8, 6, 1, 9, 8, 8,
		7, 6, -1, -1, -1, -1, 10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1,
		-1, -1, 10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1, 0, 3, 7,
		0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1, 7, 6, 10, 7, 10, 8, 8, 10,
		9, -1, -1, -1, -1, -1, -1, -1, 6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, 3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1,
		-1, 8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1, 9, 4, 6, 9,
		6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1, 6, 8, 4, 6, 11, 8, 2, 10, 1,
		-1, -1, -1, -1, -1, -1, -1, 1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1,
		-1, -1, -1, 4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1, 10,
		9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1, 8, 2, 3, 8, 4, 2, 4, 6,
		2, -1, -1, -1, -1, -1, -1, -1, 0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, 1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1, 1,
		9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1, 8, 1, 3, 8, 6, 1, 8,
		4, 6, 6, 10, 1, -1, -1, -1, -1, 10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1,
		-1, -1, -1, -1, 4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1, 10,
		9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4, 9, 5, 7, 6,
		11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 8, 3, 4, 9, 5, 11, 7, 6,
		-1, -1, -1, -1, -1, -1, -1, 5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1,
		-1, -1, -1, 11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1, 9, 5,
		4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, 6, 11, 7, 1, 2, 10,
		0, 8, 3, 4, 9, 5, -1, -1, -1, -1, 7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2,
		-1, -1, -1, -1, 3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1, 7, 2,
		3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1, 9, 5, 4, 0, 8, 6, 0, 6,
		2, 6, 8, 7, -1, -1, -1, -1, 3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1,
		-1, -1, 6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1, 9, 5, 4, 10, 1,
		6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1, 1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7,
		0, 9, 5, 4, -1, 4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1, 7,
		6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1, 6, 9, 5, 6, 11, 9,
		11, 8, 9, -1, -1, -1, -1, -1, -1, -1, 3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9,
		5, -1, -1, -1, -1, 0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1,
		-1, 6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 10, 9,
		5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1, 0, 11, 3, 0, 6, 11, 0, 9, 6,
		5, 6, 9, 1, 2, 10, -1, 11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5,
		-1, 6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1, 5, 8, 9, 5,
		2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1, 9, 5, 6, 9, 6, 0, 0, 6, 2, -1,
		-1, -1, -1, -1, -1, -1, 1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1,
		1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 3, 6, 1, 6,
		10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1, 10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0,
		-1, -1, -1, -1, 0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, 10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 11, 5,
		10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 11, 5, 10, 11, 7,
		5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1, 5, 11, 7, 5, 10, 11, 1, 9, 0,
		-1, -1, -1, -1, -1, -1, -1, 10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1,
		-1, -1, -1, 11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1, 0,
		8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1, 9, 7, 5, 9, 2, 7, 9,
		0, 2, 2, 11, 7, -1, -1, -1, -1, 7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9,
		8, 2, -1, 2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, 8, 2,
		0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1, 9, 0, 1, 5, 10, 3, 5, 3,
		7, 3, 10, 2, -1, -1, -1, -1, 9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5,
		2, -1, 1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 8,
		7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1, 9, 0, 3, 9, 3, 5, 5, 3,
		7, -1, -1, -1, -1, -1, -1, -1, 9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, 5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1,
		-1, 5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1, 0, 1, 9, 8,
		4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1, 10, 11, 4, 10, 4, 5, 11, 3,
		4, 9, 4, 1, 3, 1, 4, -1, 2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1,
		-1, -1, 0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1, 0, 2, 5,
		0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1, 9, 4, 5, 2, 11, 3, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, 2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1,
		-1, -1, 5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1, 3, 10,
		2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1, 5, 10, 2, 5, 2, 4, 1, 9, 2,
		9, 4, 2, -1, -1, -1, -1, 8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1,
		-1, -1, 0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 8, 4,
		5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1, 9, 4, 5, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, 4, 11, 7, 4, 9, 11, 9, 10, 11, -1,
		-1, -1, -1, -1, -1, -1, 0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1,
		-1, -1, 1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1, 3, 1, 4,
		3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1, 4, 11, 7, 9, 11, 4, 9, 2,
		11, 9, 1, 2, -1, -1, -1, -1, 9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0,
		8, 3, -1, 11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1, 11,
		7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1, 2, 9, 10, 2, 7, 9, 2,
		3, 7, 7, 4, 9, -1, -1, -1, -1, 9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2,
		0, 7, -1, 3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1, 1, 10,
		2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4, 9, 1, 4, 1, 7, 7,
		1, 3, -1, -1, -1, -1, -1, -1, -1, 4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1,
		-1, -1, -1, -1, 4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, 4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 9, 10,
		8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, 0, 9, 3, 9, 11,
		11, 9, 10, -1, -1, -1, -1, -1, -1, -1, 0, 1, 10, 0, 10, 8, 8, 10, 11,
		-1, -1, -1, -1, -1, -1, -1, 3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, 1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1,
		-1, 3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1, 0, 2, 11, 8,
		0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, 2, 11, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, 2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1,
		-1, -1, -1, -1, -1, 9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, 2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1, 1, 10, 2,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 3, 8, 9, 1, 8,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 9, 1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, 0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1 };

__kernel void traverseHP(
		__read_only image3d_t rawData,
		__read_only image3d_t cubeIndexes,
		__global uchar * hp0, // Largest HP
		__global uchar * hp1,
		__global ushort * hp2,
		__global ushort * hp3,
		__global ushort * hp4,
		__global int * hp5,
#if SIZE > 64
		__global int * hp6,
#endif
#if SIZE > 128
		__global int * hp7,
#endif
#if SIZE > 256
		__global int * hp8,
#endif
#if SIZE > 512
		__global int * hp9,
#endif
		__global float * VBOBuffer,
		__private int isolevel,
		__private int sum
) {

	int target = get_global_id(0);
	if(target >= sum)
	target = 0;

	int4 cubePosition = {0,0,0,0}; // x,y,z,sum
#if SIZE > 512
	cubePosition = scanHPLevel(target, hp9, cubePosition);
#endif
#if SIZE > 256
	cubePosition = scanHPLevel(target, hp8, cubePosition);
#endif
#if SIZE > 128
	cubePosition = scanHPLevel(target, hp7, cubePosition);
#endif
#if SIZE > 64
	cubePosition = scanHPLevel(target, hp6, cubePosition);
#endif
	cubePosition = scanHPLevel(target, hp5, cubePosition);
	cubePosition = scanHPLevelShort(target, hp4, cubePosition);
	cubePosition = scanHPLevelShort(target, hp3, cubePosition);
	cubePosition = scanHPLevelShort(target, hp2, cubePosition);
	cubePosition = scanHPLevelChar(target, hp1, cubePosition);
	cubePosition = scanHPLevelChar(target, hp0, cubePosition);
	cubePosition.x = cubePosition.x / 2;
	cubePosition.y = cubePosition.y / 2;
	cubePosition.z = cubePosition.z / 2;

	char vertexNr = 0;

	// max 5 triangles
	uchar cubeindex = read_imageui(cubeIndexes, sampler, cubePosition).x;
	for(int i = (target-cubePosition.s3)*3; i < (target-cubePosition.s3+1)*3; i++) { // for each vertex in triangle
		const uchar edge = triTable[cubeindex*16 + i];
		const int3 point0 = (int3)(cubePosition.x + offsets3[edge*6], cubePosition.y + offsets3[edge*6+1], cubePosition.z + offsets3[edge*6+2]);
		const int3 point1 = (int3)(cubePosition.x + offsets3[edge*6+3], cubePosition.y + offsets3[edge*6+4], cubePosition.z + offsets3[edge*6+5]);

		// Store vertex in VBO

		const float3 forwardDifference0 = (float3)(
				(float)(-read_imagei(rawData, sampler, (int4)(point0.x+1, point0.y, point0.z, 0)).x+read_imagei(rawData, sampler, (int4)(point0.x-1, point0.y, point0.z, 0)).x),
				(float)(-read_imagei(rawData, sampler, (int4)(point0.x, point0.y+1, point0.z, 0)).x+read_imagei(rawData, sampler, (int4)(point0.x, point0.y-1, point0.z, 0)).x),
				(float)(-read_imagei(rawData, sampler, (int4)(point0.x, point0.y, point0.z+1, 0)).x+read_imagei(rawData, sampler, (int4)(point0.x, point0.y, point0.z-1, 0)).x)
		);
		const float3 forwardDifference1 = (float3)(
				(float)(-read_imagei(rawData, sampler, (int4)(point1.x+1, point1.y, point1.z, 0)).x+read_imagei(rawData, sampler, (int4)(point1.x-1, point1.y, point1.z, 0)).x),
				(float)(-read_imagei(rawData, sampler, (int4)(point1.x, point1.y+1, point1.z, 0)).x+read_imagei(rawData, sampler, (int4)(point1.x, point1.y-1, point1.z, 0)).x),
				(float)(-read_imagei(rawData, sampler, (int4)(point1.x, point1.y, point1.z+1, 0)).x+read_imagei(rawData, sampler, (int4)(point1.x, point1.y, point1.z-1, 0)).x)
		);

		const int value0 = read_imagei(rawData, sampler, (int4)(point0.x, point0.y, point0.z, 0)).x;
		const float diff = native_divide(
				(float)(isolevel-value0),
				(float)(read_imagei(rawData, sampler, (int4)(point1.x, point1.y, point1.z, 0)).x - value0));

		const float3 vertex = mix((float3)(point0.x, point0.y, point0.z), (float3)(point1.x, point1.y, point1.z), diff);

		const float3 normal = mix(forwardDifference0, forwardDifference1, diff);

		vstore3(vertex, target*6 + vertexNr*2, VBOBuffer);
		vstore3(normal, target*6 + vertexNr*2 + 1, VBOBuffer);

		++vertexNr;
	}
}

__constant uchar nrOfTriangles[256] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2,
		3, 3, 2, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3, 1, 2, 2, 3, 2,
		3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3, 2, 3, 3, 2, 3, 4, 4, 3, 3, 4, 4, 3, 4,
		5, 5, 2, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3, 2, 3, 3, 4, 3,
		4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 4, 2, 3, 3, 4, 3, 4, 2, 3, 3, 4, 4, 5, 4,
		5, 3, 2, 3, 4, 4, 3, 4, 5, 3, 2, 4, 5, 5, 4, 5, 2, 4, 1, 1, 2, 2, 3, 2,
		3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3, 2, 3, 3, 4, 3, 4, 4, 5, 3, 2, 4, 3, 4,
		3, 5, 2, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 4, 3, 4, 4, 3, 4,
		5, 5, 4, 4, 3, 5, 2, 5, 4, 2, 1, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 2,
		3, 3, 2, 3, 4, 4, 5, 4, 5, 5, 2, 4, 3, 5, 4, 3, 2, 4, 1, 3, 4, 4, 5, 4,
		5, 3, 4, 4, 5, 5, 2, 3, 4, 2, 1, 2, 3, 3, 2, 3, 4, 2, 1, 3, 2, 4, 1, 2,
		1, 1, 0 };

__kernel void classifyCubes(
		__global uchar * histoPyramid,
		__global uchar * cubeIndexes,
		__read_only image3d_t rawData,
		__private int isolevel
) {
	int4 pos = {get_global_id(0), get_global_id(1), get_global_id(2), 0};

	// Find cube class nr
	const uchar first = read_imagei(rawData, sampler, pos).x;
	const uchar cubeindex =
	((first > isolevel)) |
	((read_imagei(rawData, sampler, pos + cubeOffsets[1]).x > isolevel) << 1) |
	((read_imagei(rawData, sampler, pos + cubeOffsets[3]).x > isolevel) << 2) |
	((read_imagei(rawData, sampler, pos + cubeOffsets[2]).x > isolevel) << 3) |
	((read_imagei(rawData, sampler, pos + cubeOffsets[4]).x > isolevel) << 4) |
	((read_imagei(rawData, sampler, pos + cubeOffsets[5]).x > isolevel) << 5) |
	((read_imagei(rawData, sampler, pos + cubeOffsets[7]).x > isolevel) << 6) |
	((read_imagei(rawData, sampler, pos + cubeOffsets[6]).x > isolevel) << 7);

	// Store number of triangles and index
	uint writePos = EncodeMorton3(pos.x,pos.y,pos.z);
	histoPyramid[writePos] = nrOfTriangles[cubeindex];
	cubeIndexes[pos.x+pos.y*get_global_size(0)+pos.z*get_global_size(0)*get_global_size(1)] = cubeindex;
}
