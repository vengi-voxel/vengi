#include "_random.glsl"

const float PI = 3.14159265;

void buildONB(vec3 n, out vec3 t, out vec3 b) {
	t = normalize(abs(n.y) < 0.999 ? cross(n, vec3(0.0, 1.0, 0.0)) : cross(n, vec3(1.0, 0.0, 0.0)));
	b = cross(n, t);
}

// Cosine-weighted hemisphere sampling via Malley's method.
// Reference: Shirley & Chiu, "A Low Distortion Map Between Disk and Square", 1997.
// Used to importance-sample diffuse BRDFs: pdf = cos(theta) / PI.
vec3 cosineSampleHemi(vec3 n) {
	float r1 = nextRand();
	float r2 = nextRand();
	float phi = 2.0 * PI * r1;
	float r = sqrt(r2);
	vec3 t;
	vec3 b;
	buildONB(n, t, b);
	return normalize(t * (cos(phi) * r) + b * (sin(phi) * r) + n * sqrt(max(0.0, 1.0 - r2)));
}

// Uniform cone sampling. cosMax = cos(half-angle); samples are uniform over the spherical cap.
vec3 sampleCone(vec3 dir, float cosMax) {
	float cosT = mix(1.0, cosMax, nextRand());
	float sinT = sqrt(max(0.0, 1.0 - cosT * cosT));
	float phi = 2.0 * PI * nextRand();
	vec3 t;
	vec3 b;
	buildONB(dir, t, b);
	return normalize(t * (cos(phi) * sinT) + b * (sin(phi) * sinT) + dir * cosT);
}

// Uniform point inside a unit sphere. Cube sampling with radius^(1/3) CDF inversion.
vec3 randomInSphere() {
	vec3 v = vec3(nextRand(), nextRand(), nextRand()) * 2.0 - 1.0;
	return v * pow(nextRand(), 1.0 / 3.0) / max(length(v), 1e-4);
}

// Henyey-Greenstein phase function sampling.
// g01 is remapped from [0,1] to [-0.95, 0.95]; g=0 is isotropic, g>0 forward-scattering.
// Reference: Henyey & Greenstein, "Diffuse radiation in the galaxy", ApJ 1941.
vec3 samplePhaseFunction(vec3 wo, float g01) {
	float g = clamp(g01 * 2.0 - 1.0, -0.95, 0.95);
	float u = nextRand();
	float cosT;
	if (abs(g) < 1e-3) {
		cosT = 2.0 * u - 1.0;
	} else {
		float g2 = g * g;
		float s = (1.0 - g2) / (1.0 - g + 2.0 * g * u);
		cosT = (1.0 + g2 - s * s) / (2.0 * g);
	}
	float sinT = sqrt(max(0.0, 1.0 - cosT * cosT));
	float phi = 2.0 * PI * nextRand();
	vec3 t;
	vec3 b;
	buildONB(wo, t, b);
	return normalize(t * (cos(phi) * sinT) + b * (sin(phi) * sinT) + wo * cosT);
}

float henyeyGreensteinEval(float cosT, float g) {
	float g2 = g * g;
	return (1.0 - g2) / (4.0 * PI * pow(max(1.0 + g2 - 2.0 * g * cosT, 1e-4), 1.5));
}
