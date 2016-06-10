uniform vec2 u_screensize;

vec2 calcTexCoord() {
	return gl_FragCoord.xy / u_screensize;
}
