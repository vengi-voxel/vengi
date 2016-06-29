#include "_shadow.frag"

$out vec4 o_color;
$in vec2 v_texcoord;

uniform sampler2D u_shadowmap;

void main() {
	float depth = decodeDepth($texture2D(u_shadowmap, v_texcoord));
	o_color = vec4(vec3(depth), 1.0);
}
