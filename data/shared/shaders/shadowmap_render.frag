#include "_common.frag"

$out vec4 o_color;
$in vec2 v_texcoord;

uniform float u_cascade;
uniform sampler2DArray u_shadowmap;

void main() {
	float depth = RGBAToFloat($texture2D(u_shadowmap, vec3(v_texcoord, u_cascade)));
	o_color = vec4(vec3(depth), 1.0);
}
