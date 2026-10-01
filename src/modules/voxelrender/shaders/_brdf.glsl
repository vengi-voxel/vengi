#include "_sampling.glsl"

vec3 fresnelSchlick(float cosTheta, vec3 f0) {
	return f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0);
}

float fresnelDielectric(float cosTheta, float eta) {
	float a = (eta - 1.0) / (eta + 1.0);
	return fresnelSchlick(cosTheta, vec3(a * a)).x;
}

float D_GGX(float NdotH, float a) {
	float a2 = a * a;
	float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
	return a2 / max(PI * d * d, 1e-8);
}

float lambdaSmith(float NdotX, float a) {
	float a2 = a * a;
	float n2 = NdotX * NdotX;
	return (-1.0 + sqrt(1.0 + a2 * (1.0 - n2) / max(n2, 1e-8))) * 0.5;
}

float G_Smith(float NdotV, float NdotL, float a) {
	return 1.0 / (1.0 + lambdaSmith(NdotV, a) + lambdaSmith(NdotL, a));
}

// Heitz, "Sampling the GGX Distribution of Visible Normals", JCGT 2018.
vec3 sampleGGXVNDF(vec3 Ve, float roughness) {
	float a = max(roughness * roughness, 0.001);
	vec3 Vh = normalize(vec3(a * Ve.x, a * Ve.y, max(Ve.z, 1e-6)));
	float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
	vec3 T1 = lensq > 0.0 ? vec3(-Vh.y, Vh.x, 0.0) / sqrt(lensq) : vec3(1.0, 0.0, 0.0);
	vec3 T2 = cross(Vh, T1);
	float r = sqrt(nextRand());
	float phi = 2.0 * PI * nextRand();
	float t1 = r * cos(phi);
	float t2 = r * sin(phi);
	float s = 0.5 * (1.0 + Vh.z);
	t2 = (1.0 - s) * sqrt(max(0.0, 1.0 - t1 * t1)) + s * t2;
	vec3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2)) * Vh;
	return normalize(vec3(a * Nh.x, a * Nh.y, max(0.0, Nh.z)));
}

float pdfGGXVNDF(vec3 V, vec3 L, vec3 N, float roughness) {
	vec3 H = V + L;
	float hlen = length(H);
	if (hlen < 1e-6) {
		return 0.0;
	}
	H /= hlen;
	float NdotH = max(dot(N, H), 0.0);
	float NdotV = max(dot(N, V), 1e-4);
	float a = max(roughness * roughness, 0.001);
	float D = D_GGX(NdotH, a);
	float G1 = 1.0 / (1.0 + lambdaSmith(NdotV, a));
	return (G1 * D) / (4.0 * NdotV);
}

vec3 evalBrdf(vec3 V, vec3 L, vec3 N, vec3 albedo, float roughness, float metallic) {
	float NdotL = max(dot(N, L), 0.0);
	if (NdotL <= 0.0) {
		return vec3(0.0);
	}
	float NdotV = max(dot(N, V), 1e-4);
	vec3 H = normalize(V + L);
	float NdotH = max(dot(N, H), 0.0);
	float VdotH = max(dot(V, H), 0.0);
	float a = max(roughness * roughness, 0.001);
	vec3 F0 = mix(vec3(0.04), albedo, metallic);
	vec3 F = fresnelSchlick(VdotH, F0);
	float D = D_GGX(NdotH, a);
	float G = G_Smith(NdotV, NdotL, a);
	vec3 spec = D * G * F / max(4.0 * NdotV * NdotL, 1e-4);
	vec3 diff = albedo * (1.0 - metallic) / PI;
	return diff + spec;
}
