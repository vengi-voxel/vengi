layout(binding = 0) uniform sampler2D u_accum;
layout(binding = 1) uniform sampler2D u_reveal;
$in vec2 v_texcoord;
layout(location = 0) $out vec4 o_color;

void main(void) {
	vec4 accum = $texture2D(u_accum, v_texcoord);
	float reveal = exp(min($texture2D(u_reveal, v_texcoord).r, 0.0));
	if (accum.a < 1.0e-5 && reveal > 0.999) {
		discard;
	}
	vec3 avg = accum.rgb / max(accum.a, 1.0e-5);
	float alpha = 1.0 - reveal;
	o_color = vec4(avg, alpha);
}
