flat in int v_material;
flat in int v_density;
in vec3 v_pos;
in vec3 v_diffusecolor;
in vec3 v_specularcolor;
in vec3 v_lightpos;
out vec4 o_color;

// the different material types
const int AIR = 0;
const int DIRT = 1;
const int GRASS = 2;
const int CLOUD = 3;
const int WATER = 4;
const int LEAVES = 5;
const int TRUNK = 6;
const int CLOUDS = 7;
const int MAX = 8;

vec3 materialColor[MAX] = vec3[](
	vec3(0.0),
	vec3(0.419, 0.258, 0.149),
	vec3(0.427, 0.776, 0.007),
	vec3(0.75, 0.75, 0.9),
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 0.5, 0.0),
	vec3(0.419, 0.258, 0.149),
	vec3(0.75, 0.75, 0.9)
);

const float MAX_MATERIAL = 16.0;

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
	return vec3(0.0, 0.6, 0.796);
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
	vec3 fdx = vec3(dFdx(v_pos.x), dFdx(v_pos.y), dFdx(v_pos.z));
	vec3 fdy = vec3(dFdy(v_pos.x), dFdy(v_pos.y), dFdy(v_pos.z));
	vec3 normal = normalize(cross(fdx, fdy));
	vec3 lightdir = normalize(v_lightpos - v_pos);

	vec3 materialcolor = materialColor[v_material];
	vec3 ambientcolor = getAmbientColor(materialcolor);
	vec3 diffusecolor = getDiffuseColor(lightdir, normal);
	vec3 specularcolor = getSpecularColor(lightdir, normal);
	vec3 fogcolor = getFogColor();
	vec3 finalcolor = (ambientcolor + diffusecolor + specularcolor) * materialcolor;
	o_color = vec4(mix(finalcolor, fogcolor, getFog()), 1.0);
}
