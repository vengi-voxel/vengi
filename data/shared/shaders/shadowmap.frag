#include "_shadow.frag"

$out vec4 o_color;
$in vec2 o_projZW;

void main() {
	float depth = o_projZW.x / o_projZW.y;
	o_color = vec4(0.5, 0.5, 0.5, 1.0);//encodeDepth(depth);
}
