layout(location = 0) $out vec4 o_color;
$in vec2 v_texcoord;

$constant FilterSize 3
uniform float u_coefficients[3];
uniform vec2 u_offsets[3];
uniform sampler2D u_image;

void main() {
	vec4 sum = u_coefficients[0] * $texture2D(u_image, v_texcoord + u_offsets[0]);
	sum += u_coefficients[1] * $texture2D(u_image, v_texcoord + u_offsets[1]);
	sum += u_coefficients[2] * $texture2D(u_image, v_texcoord + u_offsets[2]);

	o_color = sum;
}
