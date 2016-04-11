// Interpolated values from the vertex shaders
in vec3 voxelColor;
in vec4 worldPosition;

// Output data
out vec3 color;

void main()
{
	vec3 worldSpaceNormal = normalize(cross(dFdy(worldPosition.xyz), dFdx(worldPosition.xyz)));
	worldSpaceNormal *= -1.0; // Not sure why we have to invert this... to be checked.

	// Basic lighting calculation for overhead light.
	float ambient = 0.3;
	float diffuse = 0.7;
	vec3 lightDir = normalize(vec3(0.2, 0.8, 0.4));
	float nDotL = clamp(dot(normalize(worldSpaceNormal), lightDir), 0.0, 1.0);
	float lightIntensity = ambient + diffuse * nDotL;

	color = voxelColor * lightIntensity;
}