float sunSolidAngle() {
	return max(2.0 * PI * (1.0 - u_suncosangle), 1e-8);
}

vec3 skyRadiance(vec3 rayDir, bool withSun) {
	float height = clamp(rayDir.y, -1.0, 1.0);
	float zenithWeight = smoothstep(0.0, 1.0, height);
	float nadirWeight = smoothstep(0.0, 1.0, -height);
	vec3 col = mix(u_skyhorizon, u_skytop, zenithWeight);
	col = mix(col, u_floorcolor * 0.25, nadirWeight);
	if (withSun && dot(rayDir, u_sundir) >= u_suncosangle) {
		col += u_suncolor / sunSolidAngle();
	}
	return col;
}

// Environment importance sampling: equal-solid-angle sky bands mixed with a sun disk.
// Reference: https://pbr-book.org/4ed/Light_Sources/Infinite_Area_Lights
float skyPdf(vec3 direction) {
	int band = clamp(int((direction.y + 1.0) * 8.0), 0, 15);
	return u_skybands[band].y;
}

float environmentPdf(vec3 direction) {
	float sunPdf = dot(direction, u_sundir) >= u_suncosangle ? 1.0 / sunSolidAngle() : 0.0;
	if (all(equal(u_suncolor, vec3(0.0)))) {
		sunPdf = 0.0;
	}
	return (1.0 - u_skySamplingProbability) * sunPdf + u_skySamplingProbability * skyPdf(direction);
}

vec3 sampleEnvironment() {
	float choice = nextRand();
	if (choice >= u_skySamplingProbability) {
		return sampleCone(u_sundir, u_suncosangle);
	}
	float target = choice / u_skySamplingProbability;
	int low = 0;
	int high = 15;
	for (int step = 0; step < 4; ++step) {
		int middle = (low + high) / 2;
		if (target < u_skybands[middle].x) {
			high = middle;
		} else {
			low = middle + 1;
		}
	}
	float previous = low > 0 ? u_skybands[low - 1].x : 0.0;
	float fraction = (target - previous) / max(u_skybands[low].x - previous, 1e-12);
	float y = clamp(-1.0 + (float(low) + fraction) * 0.125, -1.0, 1.0);
	float phi = 2.0 * PI * nextRand();
	float radius = sqrt(max(0.0, 1.0 - y * y));
	return vec3(radius * cos(phi), y, radius * sin(phi));
}
