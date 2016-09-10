#include "_shadowsampler.frag"

$out vec4 o_color;
$in vec2 v_texcoord;

#if cl_depthmapformat == 2
uniform sampler2DShadow u_shadowmap;
#else
uniform sampler2D u_shadowmap;
#endif

void main() {
#if cl_depthmapformat == 0
	float depth = decodeDepth($texture2D(u_shadowmap, v_texcoord));
#elif cl_depthmapformat == 1
	float depth = $texture2D(u_shadowmap, v_texcoord).r;
#elif cl_depthmapformat == 2
	float depth = $texture2D(u_shadowmap, vec3(v_texcoord, 1.0)).r;
#endif
	o_color = vec4(vec3(depth), 1.0);
}
