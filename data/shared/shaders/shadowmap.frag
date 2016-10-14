#include "_common.frag"

$out vec4 o_color;
$in vec2 o_projZW;

void main() {
	float depth = o_projZW.x / o_projZW.y;
	o_color = floatToRGBA(depth);
}
