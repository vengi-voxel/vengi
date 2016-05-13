in vec3 v_pos;
in vec4 v_color;
in vec3 v_lightpos;
in vec3 v_diffuse_color;
in float v_fogrange;
in vec3 v_fogcolor;
in float v_viewdistance;
in float v_ambientocclusion;
in float v_debug_color;

out vec4 o_color;

void main(void) {
	vec3 fdx = dFdx(v_pos.xyz);
	vec3 fdy = dFdy(v_pos.xyz);
	vec3 normal = normalize(cross(fdx, fdy));
	vec3 lightdir = normalize(v_lightpos - v_pos);

	vec3 diffuse = v_diffuse_color * clamp(dot(normal, lightdir), 0.0, 1.0) * 0.8;
	vec3 ambient = vec3(0.2);
	vec3 lightvalue = diffuse + ambient;

	float fogstart = max(v_viewdistance - v_fogrange, 0.0);
	float fogdistance = gl_FragCoord.z / gl_FragCoord.w;
	float fogval = 1.0 - clamp((v_viewdistance - fogdistance) / (v_viewdistance - fogstart), 0.0, 1.0);

	vec3 linearColor = v_color.rgb * v_ambientocclusion * lightvalue * v_debug_color;
	o_color = vec4(mix(linearColor, v_fogcolor, fogval), v_color.a);
}
