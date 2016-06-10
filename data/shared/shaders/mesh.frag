$in vec3 v_pos;
$in vec3 v_norm;
$in vec2 v_texcoords;
$in vec3 v_lightpos;
$in float v_fogrange;
$in float v_viewdistance;
$out vec4 o_color;
uniform sampler2D u_texture;

void main(void) {
	vec3 lightdir = normalize(v_lightpos - v_pos);
	vec3 color = $texture2D(u_texture, v_texcoords).rgb;
	float diffuse = clamp(dot(v_norm, lightdir), 0.0, 1.0) * 0.7;
	float ambient = 0.3;
	float lightvalue = diffuse + ambient;

	float fogstart = max(v_viewdistance - v_fogrange, 0.0);
	float fogdistance = gl_FragCoord.z / gl_FragCoord.w;
	float fogval = 1.0 - clamp((v_viewdistance - fogdistance) / (v_viewdistance - fogstart), 0.0, 1.0);
	vec3 fogcolor = vec3(0.0, 0.6, 0.796);
	o_color = vec4(mix(color * lightvalue, fogcolor, fogval), 1.0);
}
