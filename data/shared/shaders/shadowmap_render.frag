$out vec4 o_color;
$in vec2 v_texcoord;

uniform sampler2D u_shadowmap;

void main()
{
	float v = texture(u_shadowmap, v_texcoord).r;
	o_color = vec4(vec3(v), 1.0);
}
