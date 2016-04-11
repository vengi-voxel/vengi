// Interpolated values from the vertex shaders
in vec3 worldSpaceNormal;
in vec4 materialWeights;

// Output data
out vec3 color;

uniform uint height;

void main()
{
	// Vertex colors coming out of Cubiquity don't actually sum to one
	// (roughly 0.5 as that's where the isosurface is). Make them sum
	// to one, though Cubiquity should probably be changed to do this.
	// Even when fixed, interpolation could cause the sum to be wrong.
	float sumOfMaterialWeights = dot(materialWeights, vec4(1.0, 1.0, 1.0, 1.0));
	vec4 normalizedMaterialWeights = materialWeights / sumOfMaterialWeights;

	// Colors taken from https://en.wikipedia.org/wiki/List_of_colors
	vec4 material0 = vec4(0.88, 0.66, 0.37, 1.0); // Earth yellow
	vec4 material1 = vec4(0.09, 0.55, 0.09, 1.0); // Forest green
	vec4 material2 = vec4(0.00, 0.00, 1.00, 1.0); // Blue
	vec4 material3 = vec4(1.00, 1.00, 1.00, 1.0); // White

	vec4 blendedColor =
		material0 * normalizedMaterialWeights.x +
		material1 * normalizedMaterialWeights.y +
		material2 * normalizedMaterialWeights.z +
		material3 * normalizedMaterialWeights.w;

	if(height == 0u)
	{
		blendedColor = vec4(1.0, 0.5, 0.5, 1.0);
	}
	else if(height == 1u)
	{
		blendedColor = vec4(0.5, 1.0, 0.5, 1.0);
	}
	else if(height == 2u)
	{
		blendedColor = vec4(0.5, 0.5, 1.0, 1.0);
	}
	else if(height == 3u)
	{
		blendedColor = vec4(1.0, 1.0, 1.0, 1.0);
	}

	// Basic lighting calculation for overhead light.
	float ambient = 0.3;
	float diffuse = 0.7;
	vec3 lightDir = vec3(0.0, 1.0, 0.0);
	float nDotL = clamp(dot(normalize(worldSpaceNormal), lightDir), 0.0, 1.0);
	float lightIntensity = ambient + diffuse * nDotL;

	// Output color = color of the texture at the specified UV
	color = vec3(blendedColor.xyz * lightIntensity);
}