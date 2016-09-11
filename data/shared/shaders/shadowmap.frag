#include "_shadowsampler.frag"

#if cl_depthmapformat == 0
$out vec4 o_color;
$in vec2 o_projZW;
#endif

void main() {
#if cl_depthmapformat == 0
	float depth = o_projZW.x / o_projZW.y;
	o_color = encodeDepth(depth);
#endif
}
