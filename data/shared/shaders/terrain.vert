// We pack the encoded position and the encoded normal into a single
// vertex attribute to save space: http://stackoverflow.com/a/21680009
in uvec4 encodedPositionAndNormal;
in uvec4 materialWeightsAsUBytes;

// Output data
out vec3 worldSpaceNormal;
out vec4 materialWeights;

// Values that stay constant for the whole mesh.
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

vec3 decodePosition(uvec3 encodedPosition)
{
	return vec3(encodedPosition) / 256.0;
}

// Returns +/- 1
vec2 signNotZero(vec2 v)
{
	return vec2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
}

vec3 decodeNormal(uint encodedNormal)
{
	//Get the encoded bytes of the normal
	uint encodedX = (encodedNormal >> 8u) & 0xFFu;
	uint encodedY = (encodedNormal) & 0xFFu;

	// Map to range [-1.0, +1.0]
	vec2 e = vec2(encodedX, encodedY);
	e = e * vec2(1.0 / 127.5, 1.0 / 127.5);
	e = e - vec2(1.0, 1.0);

	// Now decode normal using listing 2 of http://jcgt.org/published/0003/02/01/
	vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
	if (v.z < 0)
	{
		v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
	}
	v = normalize(v);
	return v;
}

void main()
{
	// Extract and decode the position.
	vec3 modelSpacePosition = decodePosition(encodedPositionAndNormal.xyz);

	// Extract and decode the normal.
	vec3 modelSpaceNormal = decodeNormal(encodedPositionAndNormal.w);
	worldSpaceNormal = modelSpaceNormal; // Valid if we don't scale or rotate our volume.

	// Pass through the material weights.
	materialWeights = materialWeightsAsUBytes;

	// Output position of the vertex in clip space.
	gl_Position =  projectionMatrix * viewMatrix * modelMatrix * vec4(modelSpacePosition,1);
}
