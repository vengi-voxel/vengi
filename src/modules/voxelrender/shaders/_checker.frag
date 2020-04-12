// https://thebookofshaders.com
float random (in vec2 st) {
	return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

float checker(in vec2 pos) {
	vec2 c = floor(pos);
	float variance = 0.05 * random(c);
	float checker = mod(c.x + c.y, 2.0);
	return mix(0.95 + variance, 1.0 - variance, checker);
}

vec3 checkerBoardColor(in vec3 normal, in vec3 pos, in vec3 color) {
	float checkerBoardFactor = 1.0;
	if (abs(normal.y) >= 0.999) {
		checkerBoardFactor = checker(pos.xz);
	} else if (abs(normal.x) >= 0.999) {
		checkerBoardFactor = checker(pos.yz);
	} else if (abs(normal.z) >= 0.999) {
		checkerBoardFactor = checker(pos.xy);
	}
	return color * checkerBoardFactor;
}
