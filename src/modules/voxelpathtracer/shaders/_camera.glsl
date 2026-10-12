// Shirley-Chiu concentric square-to-disk mapping.
// https://pbr-book.org/4ed/Sampling_Algorithms/Sampling_Multidimensional_Functions
vec2 sampleLensDisk(vec2 samplePoint) {
	vec2 offset = 2.0 * samplePoint - 1.0;
	if (all(equal(offset, vec2(0.0)))) {
		return vec2(0.0);
	}
	bool horizontal = abs(offset.x) > abs(offset.y);
	float radius = horizontal ? offset.x : offset.y;
	float angle = horizontal ? (PI * 0.25) * offset.y / offset.x
		: PI * 0.5 - (PI * 0.25) * offset.x / offset.y;
	return radius * vec2(cos(angle), sin(angle));
}

void sampleCameraRay(vec2 jitter, uint pixelSeed, int sampleIndex, out vec3 rayOrigin, out vec3 rayDir) {
	// Inverse-CDF sampling of a tent filter with a one-pixel radius.
	// https://pbr-book.org/4ed/Sampling_Algorithms/Sampling_1D_Functions
	vec2 centered = 2.0 * jitter - 1.0;
	vec2 pixelOffset = sign(centered) * (1.0 - sqrt(1.0 - abs(centered)));
	vec2 pixelUv = (gl_FragCoord.xy + pixelOffset) / u_resolution;
#ifdef CLIPORIGINUPPERLEFT
	pixelUv.y = 1.0 - pixelUv.y;
#endif
	vec2 ndc = pixelUv * 2.0 - 1.0;
#ifdef CLIPDEPTHZ0TO1
	float zNear = 0.0;
#else
	float zNear = -1.0;
#endif
	vec4 farH = u_invviewproj * vec4(ndc, 1.0, 1.0);
	vec3 farP = farH.xyz / farH.w;
	rayOrigin = u_camerapos;
	if (u_ortho != 0) {
		vec4 nearH = u_invviewproj * vec4(ndc, zNear, 1.0);
		rayOrigin = nearH.xyz / nearH.w;
	}
	// Perspective near and far points lie on the same ray from the camera eye.
	rayDir = normalize(farP - rayOrigin);

	if (u_aperture > 0.0 && u_ortho == 0) {
		float focusDist = u_focusdistance / max(dot(rayDir, u_cameraforward), 1e-3);
		vec3 focus = u_camerapos + rayDir * focusDist;
		vec2 lens = sampleLensDisk(cameraSobol(pixelSeed, sampleIndex, 1u)) * u_aperture;
		rayOrigin = u_camerapos + u_cameraright * lens.x + u_cameraup * lens.y;
		rayDir = normalize(focus - rayOrigin);
	}
}
