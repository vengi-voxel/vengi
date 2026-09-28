$in vec4 v_color;
noperspective $in float v_edge;
noperspective $in float v_half;

layout(location = 0) $out vec4 o_color;
layout(location = 1) $out vec4 o_glow;

void main() {
	float dist = abs(v_edge);
	float w = max(fwidth(dist), 0.5);
	float alpha = 1.0 - smoothstep(v_half, v_half + w, dist);
	alpha *= v_color.a;
	o_color = vec4(v_color.rgb, alpha);
	o_glow = vec4(0.0, 0.0, 0.0, 0.0);
}
