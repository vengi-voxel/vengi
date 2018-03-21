#if cl_fog == 1
uniform lowp vec3 u_fogcolor;
uniform float u_viewdistance;
$in float v_fogdivisor;

vec4 fog(in vec3 linearColor, in float alpha) {
	float fogdistance = gl_FragCoord.z / gl_FragCoord.w;
	float fogval = 1.0 - clamp((u_viewdistance - fogdistance) / v_fogdivisor, 0.0, 1.0);
	return vec4(mix(linearColor, u_fogcolor, fogval), alpha);
}

#else // cl_fog

vec4 fog(in vec3 linearColor, in float alpha) {
	return vec4(linearColor, alpha);
}

#endif // cl_fog
