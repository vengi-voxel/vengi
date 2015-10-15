in vec3 v_pos;
in vec3 v_norm;
in vec2 v_texcoords;
in vec3 v_diffusecolor;
in vec3 v_specularcolor;
in vec3 v_lightpos;
out vec4 o_color;
uniform sampler2D u_texture;

vec3 getMaterialColor() {
	return texture(u_texture, v_texcoords).rgb;
}

vec3 getAmbientColor(vec3 materialcolor) {
	const float ambientstrength = 1.0;
	return ambientstrength * materialcolor;
}

vec3 getDiffuseColor(vec3 lightdir, vec3 normal) {
	float diffuse = max(dot(lightdir, normal), 0.0);
	return diffuse * v_diffusecolor;
}

vec3 getSpecularColor(vec3 lightdir, vec3 normal) {
	const float specularstrength = 0.5;
	vec3 viewdir = normalize(-v_pos);
	vec3 reflectdir = reflect(-lightdir, normal);
	float specangle = max(dot(reflectdir, viewdir), 0.0);
	float spec = pow(specangle, 32);
	return specularstrength * spec * v_specularcolor;
}

vec3 getFogColor() {
	return vec3(0.04, 0.29, 0.94);
}

float getFog() {
	const float density = 0.05;
	const float endfog = 1500.0;
	const float startfog = 500.0;
	float fogdistance = gl_FragCoord.z / gl_FragCoord.w;
	float d = density * fogdistance;
	float fogval = (endfog - fogdistance) / (endfog - startfog);
	return 1.0 - clamp(fogval, 0.0, 1.0);
}

void main(void) {
	vec3 lightdir = normalize(v_lightpos - v_pos);

	vec3 materialcolor = getMaterialColor();
	vec3 ambientcolor = getAmbientColor(materialcolor);
	vec3 diffusecolor = getDiffuseColor(lightdir, v_norm);
	vec3 specularcolor = getSpecularColor(lightdir, v_norm);
	vec3 fogcolor = getFogColor();
	vec3 finalcolor = (ambientcolor + diffusecolor + specularcolor) * materialcolor;
	o_color = vec4(mix(finalcolor, fogcolor, getFog()), 1.0);
}
