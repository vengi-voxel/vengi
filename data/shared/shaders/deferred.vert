$in uvec3 a_pos;
$in uvec2 a_info;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform sampler2D u_texture;
uniform vec4 u_materialcolor[32];

$out vec3 v_pos;
$out vec4 v_color;

void main(void) {
	uint a_material = a_info[1];
	vec4 pos4 = u_model * vec4(a_pos, 1.0);
	v_pos = pos4.xyz;

	vec3 materialColor = u_materialcolor[a_material].rgb;
	vec3 colornoise = texture(u_texture, abs(pos4.xz) / 256.0 / 10.0).rgb;
	v_color = vec4(materialColor * colornoise * 1.8, u_materialcolor[a_material].a);
	v_color = clamp(v_color, 0.0, 1.0);

	gl_Position = u_projection * u_view * pos4;
}
