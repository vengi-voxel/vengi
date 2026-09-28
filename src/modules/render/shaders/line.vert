layout(location = 0) $in vec4 a_start;
layout(location = 1) $in vec4 a_end;
layout(location = 2) $in vec4 a_color;

layout(std140, binding = 0) uniform u_vert {
	mat4 u_viewprojection;
	mat4 u_model;
	// xy = viewport in pixels, z = line width in pixels
	vec4 u_viewport;
};

$out vec4 v_color;
noperspective $out float v_edge;
noperspective $out float v_half;

void main() {
	vec4 clip0 = u_viewprojection * (u_model * vec4(a_start.xyz, 1.0));
	vec4 clip1 = u_viewprojection * (u_model * vec4(a_end.xyz, 1.0));
	float w0 = clip0.w;
	float w1 = clip1.w;
	if (abs(w0) < 1.0e-5) {
		w0 = 1.0e-5;
	}
	if (abs(w1) < 1.0e-5) {
		w1 = 1.0e-5;
	}

	vec2 screen = max(u_viewport.xy, vec2(1.0));
	vec2 ndc0 = clip0.xy / w0;
	vec2 ndc1 = clip1.xy / w1;
	vec2 dirPx = (ndc1 - ndc0) * screen * 0.5;
	float len = length(dirPx);
	vec2 n = (len > 1.0e-3) ? vec2(-dirPx.y, dirPx.x) / len : vec2(0.0, 1.0);

	float corner = a_start.w;
	float along = step(1.5, corner);
	float side = (corner < 0.5 || corner > 2.5) ? -1.0 : 1.0;
	float halfPixels = max(u_viewport.z, 1.0) * 0.5;
	float expand = halfPixels + 0.75;

	vec4 clip = mix(clip0, clip1, along);
	vec2 offsetNdc = n * side * expand * 2.0 / screen;
	clip.xy += offsetNdc * clip.w;

	v_color = a_color;
	v_edge = side * expand;
	v_half = halfPixels;
	gl_Position = clip;
}
