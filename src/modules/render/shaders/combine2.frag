uniform sampler2D u_texture0;
uniform sampler2D u_texture1;
$in vec2 v_texcoord;
layout(location = 0) $out vec4 o_color;

void main(void) {
	o_color = $texture2D(u_texture0, v_texcoord) + $texture2D(u_texture1, v_texcoord);
}
