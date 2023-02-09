layout(location = 0) $out vec4 o_color;
$in vec2 v_texcoord;

$constant FilterSize 3
layout(std140) uniform u_frag {
	float u_coefficients0;
	float u_coefficients1;
	float u_coefficients2;
	vec2 u_offsets0;
	vec2 u_offsets1;
	vec2 u_offsets2;
};
uniform sampler2D u_image;

void main() {
	vec4 sum = u_coefficients0 * $texture2D(u_image, v_texcoord + u_offsets0);
	sum += u_coefficients1 * $texture2D(u_image, v_texcoord + u_offsets1);
	sum += u_coefficients2 * $texture2D(u_image, v_texcoord + u_offsets2);

	o_color = sum;
}
