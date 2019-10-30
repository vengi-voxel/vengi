#if cl_fog == 1
uniform lowp vec3 u_fogcolor;
uniform lowp vec3 u_focuspos;
uniform float u_fogrange;

vec4 fog(in vec3 pos, in vec3 linearColor, in float alpha) {
	float minFog = 0.7;
	float maxFog = 1.0;
	float fog = distance(pos.xyz, u_focuspos.xyz) / u_fogrange;
	float fogVal = pow(clamp((fog - minFog) / (maxFog - minFog), 0.0, 1.0), 1.7);
	return vec4(mix(linearColor, u_fogcolor, fogVal), alpha);
}

#else // cl_fog

vec4 fog(in vec3 pos, in vec3 linearColor, in float alpha) {
	return vec4(linearColor, alpha);
}

#endif // cl_fog
