$in vec3 v_pos;
$in vec3 v_norm;
$in vec2 v_texcoords;
$in vec3 v_lightpos;
$in vec4 v_color;
$out vec4 o_color;
uniform sampler2D u_texture;
uniform float u_fogrange;
uniform float u_viewdistance;
uniform vec3 u_lightpos;

void main(void) {
	vec3 lightdir = normalize(u_lightpos - v_pos);
	vec3 color = $texture2D(u_texture, v_texcoords).rgb + v_color.rgb;
	float diffuse = clamp(dot(v_norm, lightdir), 0.0, 1.0) * 0.7;
	float ambient = 0.3;
	float lightvalue = diffuse + ambient;

	float fogstart = max(u_viewdistance - u_fogrange, 0.0);
	float fogdistance = gl_FragCoord.z / gl_FragCoord.w;
	float fogval = 1.0 - clamp((u_viewdistance - fogdistance) / (u_viewdistance - fogstart), 0.0, 1.0);
	vec3 fogcolor = vec3(0.0, 0.6, 0.796);
	o_color = vec4(mix(color * lightvalue, fogcolor, fogval), 1.0);
}
