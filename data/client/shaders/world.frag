in vec3 v_pos;
in vec3 v_color;
in float v_fogrange;
in float v_viewdistance;
in float v_ambientocclusion;

out vec4 o_color;

void main(void) {
	float fogstart = max(v_viewdistance - v_fogrange, 0.0);
	float fogdistance = gl_FragCoord.z / gl_FragCoord.w;
	float fogval = 1.0 - clamp((v_viewdistance - fogdistance) / (v_viewdistance - fogstart), 0.0, 1.0);
	vec3 fogcolor = vec3(0.0, 0.6, 0.796);
	o_color = vec4(mix(v_color * v_ambientocclusion, fogcolor, fogval), 1.0);
}
