uniform lowp vec3 u_night_color;

const float dayLength = 60.0f * 60.0f * 24.0f;
const float secondsFactor = 1.0 / dayLength;

vec3 dayTimeColor(in vec3 ambientColor, float dayTimeSeconds) {
	float timeOfDay = 0.5 * sin(dayTimeSeconds * secondsFactor) + 0.5;
	vec3 color = mix(u_night_color, ambientColor, timeOfDay + 0.1);
	return color;
}