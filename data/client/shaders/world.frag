in vec3 v_pos;
in vec3 v_color;
in vec3 v_lightpos;
in float v_fogrange;
in float v_viewdistance;
in float v_ambientocclusion;

out vec4 o_color;

void main(void) {
	vec3 fdx = vec3(dFdx(v_pos.x), dFdx(v_pos.y), dFdx(v_pos.z));
	vec3 fdy = vec3(dFdy(v_pos.x), dFdy(v_pos.y), dFdy(v_pos.z));
	vec3 normal = normalize(cross(fdx, fdy));
	vec3 lightdir = normalize(v_lightpos - v_pos);

	float diffuse = clamp(dot(normal, lightdir), 0.0, 1.0) * 0.7;
	float ambient = 0.3;
	float lightvalue = diffuse + ambient;

	float fogstart = max(v_viewdistance - v_fogrange, 0.0);
	float fogdistance = gl_FragCoord.z / gl_FragCoord.w;
	float fogval = 1.0 - clamp((v_viewdistance - fogdistance) / (v_viewdistance - fogstart), 0.0, 1.0);
	vec3 fogcolor = vec3(0.0, 0.6, 0.796);
	o_color = vec4(mix(v_color * v_ambientocclusion * lightvalue, fogcolor, fogval), 1.0);
}
