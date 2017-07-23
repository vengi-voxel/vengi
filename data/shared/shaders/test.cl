struct Data {
	int foo;
	char foo2;
	float foo3;
	float4 foo4;
};

__kernel void example( __global const char* buf, __global char* buf2 ) {
	int x = get_global_id(0);
	buf2[x] = buf[x];
}

__kernel void example2( __global const char* buf, __global char* buf2, int N ) {
	int x = get_global_id(0);
	buf2[x] = buf[x];
}

__kernel void vector_add(__global const int *A, __global const int *B, __global int *C) {
	// Get the index of the current element
	int i1 = get_global_id(0);

	// Do the operation
	C[i1] = A[i1] + B[i1];
}
