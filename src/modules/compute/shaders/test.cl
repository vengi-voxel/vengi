// this file does contain the hardest bug class - whitespace errors... this is
// on purpose to check the parser/tokenizer

	/** multiline
	next line
   */

	// some comment to skip

/**
 * @brief useful Doxygen comment
 * @note Next line
 **/
struct Data {
	int foo_int32_t;
	char foo2_int8_t;
	char4 foo2_char4;
	float foo3_float;
	/**
	 * multiline comment
	 */
	float2 foo3_vec2;
	// some comment
	float3 foo3_vec4;
	float4 foo4_vec4;
	float4 foo4_vec4_2[2];
} /* inline */ ;

 __kernel void example( __global  const char* buf, __global char* buf2 )
  {
	int x = get_global_id(0);
	buf2[x] = buf[x];
}

__kernel  void  example2 ( __global const char *  buf ,  __global  char* buf2, int N) {
	int x = get_global_id(0);
	buf2[x] = buf[x];
}

__kernel void exampleVectorAddInt(__global const int *A, __global const int *B, __global int *C) {
	// some comment to skip
	int i1 = get_global_id(0);

	/** multiline
		next line
		*/
	C[i1] = A[i1] + B[i1];
}

__kernel void exampleVectorAddFloat3(__global const float3 *A, __global const float3 *B, __global float3 *C) {
	int i1 = get_global_id(0);
	C[i1] = A[i1] + B[i1];
}

__kernel void exampleVectorAddFloat3NoPointer(const float3 A, const float3 B, float3 C) {
	int i1 = get_global_id(0);
}

__kernel void examplePointertest(__global const float*A, __global const float*B, __global float3*C) {
	int i1 = get_global_id(0);
	C[i1] = A[i1] + B[i1];
}

__kernel void exampleLocal(__local const char *  bufLocal, __global const char* buf, __global char* buf2) {
	int x = get_global_id(0);
	buf2[x] = buf[x] + bufLocal[x];
}

__kernel void exampleDataStruct(const __global struct Data* data, __global struct Data* out) {
	out->foo_int32_t = data->foo_int32_t;
}

__kernel void exampleNoise(__global uchar4 *output, const float2 pos, float frequency, float lacunarity, int octaves, float amplitude) {
}

__kernel void exampleNoiseOther(__global uchar4 *output, __global const float2* pos, float4 frequency, uchar4 lacunarity, int4 octaves, uint3 amplitude) {
}

__kernel void exampleVec2Vector(__global float2 *output) {
}

__kernel void exampleVec3Vector(__global float3 *output) {
}

__kernel void exampleVec4Vector(__global float4 *output) {
}

__kernel void exampleIVec2Vector(__global int2 *output) {
}

__kernel void exampleIVec3Vector(__global int3 *output) {
}

__kernel void exampleIVec4Vector(__global int4 *output) {
}

__kernel void exampleUVec2Vector(__global uchar2 *output) {
}

__kernel void exampleUVec3Vector(__global uchar3 *output) {
}

__kernel void exampleUVec4Vector(__global uchar4 *output) {
}
