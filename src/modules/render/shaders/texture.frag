layout(binding = 0) uniform sampler2D u_texture;
$in vec2 v_texcoord;
layout(location = 0) $out vec4 o_color;

void main(void) {
	o_color = $texture2D(u_texture, v_texcoord);
}
