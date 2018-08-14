#if 0
__constant sampler_t volumeSampler =
      CLK_NORMALIZED_COORDS_FALSE
    | CLK_ADDRESS_CLAMP_TO_EDGE
    | CLK_FILTER_NEAREST;
#endif

__kernel void render(__read_only image3d_t volume, sampler_t volumeSampler, __global uchar *output, uint imageWidth, uint imageHeight, float w) {
	uint x = get_global_id(0);
	uint y = get_global_id(1);

	float u = x / (float) imageWidth;
	float v = y / (float) imageHeight;

	uint4 voxel = read_imageui(volume, volumeSampler, (float4)(u, v, w, 1.0f));

	if (x < imageWidth && y < imageHeight) {
		uint i = (((imageHeight - 1) - y) * imageWidth * 4) + x * 4;
		output[i + 0] = output[i + 1] = output[i + 2] = 0;
		output[i + 3] = 255;
		if (voxel[0] == 0) { // empty
			output[i + 2] = 255;
		} else if (voxel[0] == 3) { // grass
			output[i + 1] = 255;
		} else if (voxel[0] == 14) { // dirt
			output[i + 0] = 127;
			output[i + 1] = 64;
		} else { // unknown 0xff00ffff
			output[i + 0] = 255;
			output[i + 1] = 0;
			output[i + 2] = 255;
		}
	}
}
