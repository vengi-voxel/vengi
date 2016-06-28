#include "_shadow.frag"

$out vec4 o_color;
$in vec2 v_texcoord;
uniform float u_near;
uniform float u_far;

uniform sampler2D u_shadowmap;

float linearizeDepth(float depth) {
	return (2.0 * u_near) / (u_far + u_near - depth * (u_far - u_near));
}

void main() {
	float depth = linearizeDepth(decodeDepth($texture2D(u_shadowmap, v_texcoord)));
	o_color = vec4(vec3(depth), 1.0);
}
