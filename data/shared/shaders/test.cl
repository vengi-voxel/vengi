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
	int foo;
	char foo2;
	float foo3;
	float4 foo4;
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

__kernel void exampleVectorAddFloat3NoPointer(__global const float3 A, __global const float3 B, __global float3 C) {
	int i1 = get_global_id(0);
	C[i1] = A[i1] + B[i1];
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
	out->foo = data->foo;
}
