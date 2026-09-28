$in vec3 v_pos;

layout(std140, binding = 1) uniform u_frag {
	vec4 u_mins;
	vec4 u_step;
	vec4 u_color;
	vec4 u_majorcolor;
};

layout(location = 0) $out vec4 o_color;
layout(location = 1) $out vec4 o_glow;

// Screen-space pixel distance via |grad|, not L1 fwidth (too wide on diagonals).
float derivLen(float coord) {
	return length(vec2(dFdx(coord), dFdy(coord)));
}

// ~1.5px line, faded per-axis when cells drop below ~2px to kill grazing moire.
float axisLine(float coord) {
	float w = max(derivLen(coord), 1.0e-8);
	float fade = 1.0 - smoothstep(0.22, 0.55, w);
	float dist = abs(fract(coord - 0.5) - 0.5);
	float halfWidth = 0.75 * w;
	float aa = 0.75 * w;
	return (1.0 - smoothstep(halfWidth, halfWidth + aa, dist)) * fade;
}

void main() {
	vec3 faceN = abs(cross(dFdx(v_pos), dFdy(v_pos)));
	vec2 f;
	vec2 stepxy;
	vec2 origin;
	if (faceN.z >= faceN.x && faceN.z >= faceN.y) {
		f = v_pos.xy;
		stepxy = u_step.xy;
		origin = u_mins.xy;
	} else if (faceN.x >= faceN.y) {
		f = v_pos.yz;
		stepxy = u_step.yz;
		origin = u_mins.yz;
	} else {
		f = v_pos.xz;
		stepxy = u_step.xz;
		origin = u_mins.xz;
	}
	stepxy = max(stepxy, vec2(1.0e-4));
	vec2 cell = (f - origin) / stepxy;
	float majorStride = max(u_step.w, 1.0);
	float minorX = axisLine(cell.x);
	float minorY = axisLine(cell.y);
	float majorX = axisLine(cell.x / majorStride);
	float majorY = axisLine(cell.y / majorStride);
	float minor = 1.0 - (1.0 - minorX) * (1.0 - minorY);
	float major = 1.0 - (1.0 - majorX) * (1.0 - majorY);
	float coverage = max(minor, major);
	if (coverage < 0.002) {
		o_color = vec4(0.0);
		o_glow = vec4(0.0);
		return;
	}
	float majorMix = clamp(major / max(coverage, 1.0e-4), 0.0, 1.0);
	vec3 rgb = mix(u_color.rgb, u_majorcolor.rgb, majorMix);
	o_color = vec4(rgb, coverage * max(u_color.a, u_majorcolor.a));
	o_glow = vec4(0.0, 0.0, 0.0, 0.0);
}
