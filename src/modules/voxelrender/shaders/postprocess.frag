uniform sampler2D u_texture;
$in vec2 v_texcoord;
uniform vec4 u_color;
$out vec4 o_color;

void main(void) {
	vec4 color = $texture2D(u_texture, v_texcoord);
	o_color = vec4(color.rgb * u_color.rgb, 1.0);
}
