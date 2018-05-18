uniform sampler2D u_texture;
$in vec2 v_texcoord;
$in vec4 v_color;
$out vec4 o_color;

void main(void) {
	vec4 color = $texture2D(u_texture, v_texcoord);
	vec4 fcolor = v_color / 255.0;
	o_color = color * fcolor * 255.0;
}
