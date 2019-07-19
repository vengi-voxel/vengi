uniform sampler2D u_texture;
$in vec2 v_texcoord;
$in vec4 v_color;
$out vec4 o_color;

const float texOffset = 1.0 / 100.0;

void main() {
	vec2 tc0 = v_texcoord.st + vec2(-texOffset, -texOffset);
	vec2 tc1 = v_texcoord.st + vec2(       0.0, -texOffset);
	vec2 tc2 = v_texcoord.st + vec2(+texOffset, -texOffset);
	vec2 tc3 = v_texcoord.st + vec2(-texOffset,        0.0);
	vec2 tc4 = v_texcoord.st + vec2(       0.0,        0.0);
	vec2 tc5 = v_texcoord.st + vec2(+texOffset,        0.0);
	vec2 tc6 = v_texcoord.st + vec2(-texOffset, +texOffset);
	vec2 tc7 = v_texcoord.st + vec2(       0.0, +texOffset);
	vec2 tc8 = v_texcoord.st + vec2(+texOffset, +texOffset);

	vec4 col = vec4(0.0);
	col += $texture2D(u_texture, tc0);
	col += $texture2D(u_texture, tc1);
	col += $texture2D(u_texture, tc2);
	col += $texture2D(u_texture, tc3);
	vec4 textureColor = $texture2D(u_texture, tc4);
	col += textureColor;
	col += $texture2D(u_texture, tc5);
	col += $texture2D(u_texture, tc6);
	col += $texture2D(u_texture, tc7);
	col += $texture2D(u_texture, tc8);

	vec4 sum = 8.0 * textureColor - col;
	o_color = vec4(sum.rgb, 1.0) * v_color;
}
