#include "_shadowmap.frag"

$out vec4 o_color;
$in vec2 v_texcoord;

void main() {
	float depth = decodeDepth($texture2D(u_shadowmap, v_texcoord));
	o_color = vec4(calculateShadowUV(), 0.0, depth + 1.0);
}
