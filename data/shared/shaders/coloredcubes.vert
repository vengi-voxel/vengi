in uvec4 encodedPosition;
in uint quantizedColor;

// Output data
out vec3 voxelColor;
out vec4 worldPosition;

// Values that stay constant for the whole mesh.
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

uint MaxInOutValue = 255u;

uint RedMSB = 31u;
uint RedLSB = 27u;
uint GreenMSB = 26u;
uint GreenLSB = 21u;
uint BlueMSB = 20u;
uint BlueLSB = 16u;
uint AlphaMSB = 15u;
uint AlphaLSB = 12u;

uint NoOfRedBits = RedMSB - RedLSB + 1u;
uint NoOfGreenBits = GreenMSB - GreenLSB + 1u;
uint NoOfBlueBits = BlueMSB - BlueLSB + 1u;
uint NoOfAlphaBits = AlphaMSB - AlphaLSB + 1u;

uint RedScaleFactor = MaxInOutValue / ((0x01u << NoOfRedBits) - 1u);
uint GreenScaleFactor = MaxInOutValue / ((0x01u << NoOfGreenBits) - 1u);
uint BlueScaleFactor = MaxInOutValue / ((0x01u << NoOfBlueBits) - 1u);
uint AlphaScaleFactor = MaxInOutValue / ((0x01u << NoOfAlphaBits) - 1u);

vec3 decodePosition(uvec3 encodedPosition)
{
	return vec3(encodedPosition) - 0.5;
}

vec3 decodeColor(uint quantizedColor)
{
	quantizedColor >>= 16;
	float blue = (quantizedColor & 0x1Fu) * BlueScaleFactor;
	quantizedColor >>= 5;
	float green = (quantizedColor & 0x3Fu) * GreenScaleFactor;
	quantizedColor >>= 6;
	float red = (quantizedColor & 0x1Fu) * RedScaleFactor;

	vec3 result = vec3(red, green, blue);
	result *= (1.0 / 255.0);
	return result;
}

void main()
{
	// Extract and decode the position.
	vec3 modelSpacePosition = decodePosition(encodedPosition.xyz);

	// Extract and decode the color.
	voxelColor = decodeColor(quantizedColor);

	// We pass the world-space position through to the fragment
	// shader because we will use it to compute the normal.
	worldPosition = modelMatrix * vec4(modelSpacePosition, 1);

	// Output position of the vertex in clip space.
	gl_Position =  projectionMatrix * viewMatrix * worldPosition;
}
