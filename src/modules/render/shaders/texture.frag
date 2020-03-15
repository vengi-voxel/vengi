uniform sampler2D u_texture;
$in vec2 v_texcoord;
$in vec4 v_color;
layout(location = 0) $out vec4 o_color;

void main(void) {
	vec4 color = $texture2D(u_texture, v_texcoord);
	o_color = color * v_color;
}
