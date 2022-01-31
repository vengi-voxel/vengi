layout(location = 0) $out vec4 o_color;
$in vec2 v_texcoord;

uniform sampler2D u_image;
uniform bool u_horizontal;

void main() {
	vec2 offset = 1.0 / textureSize(u_image, 0);
	float u_weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
	vec4 result = $texture2D(u_image, v_texcoord) * u_weight[0];
	if (u_horizontal) {
		for (int i = 1; i < 5; ++i) {
			vec2 step_offset = vec2(offset.x * i, 0.0);
			result += $texture2D(u_image, v_texcoord + step_offset) * u_weight[i];
			result += $texture2D(u_image, v_texcoord - step_offset) * u_weight[i];
		}
	} else {
		for (int i = 1; i < 5; ++i) {
			vec2 step_offset = vec2(0.0, offset.y * i);
			result += $texture2D(u_image, v_texcoord + step_offset) * u_weight[i];
			result += $texture2D(u_image, v_texcoord - step_offset) * u_weight[i];
		}
	}
	o_color = result;
}