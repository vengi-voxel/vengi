$in vec4 v_color;
$in vec3 v_normal;
$in vec4 v_fragpos;
$in vec4 v_lightpos;
$in vec4 v_lightcolor;

layout(location = 0) $out vec4 o_color;
layout(location = 1) $out vec4 o_glow;

void main()
{
	float ambientStrength = v_lightcolor.w;
	if (ambientStrength > 0.0) {
		vec3 lightColor = v_lightcolor.rgb;
		vec3 lightDir = normalize(vec3(v_lightpos - v_fragpos));
		float diff = max(dot(v_normal, lightDir), 0.0);
		vec3 diffuse = vec3(diff, diff, diff) * lightColor;
		vec3 ambient = lightColor * vec3(ambientStrength, ambientStrength, ambientStrength);
		vec3 result = (ambient + diffuse) * v_color.rgb;
		o_color = vec4(result, v_color.a);
	} else {
		o_color = v_color;
	}
	o_glow = vec4(0.0, 0.0, 0.0, 0.0);
}
