#version 330

uniform vec3 colour;
uniform vec3 lightColour;

in vec3 fragPositionView;
in vec3 lightPositionView;
in vec3 normalView;

out vec4 fragColor;

void main() {
	// direction from fragment to light
	vec3 l = normalize(lightPositionView - fragPositionView);

	// Cosine of the angle between the normal and the light direction,
	// clamped above 0
	//  - light is at the vertical of the triangle -> 1
	//  - light is perpendicular to the triangle -> 0
	//  - light is behind the triangle -> 0
	float cosTheta = max( dot(normalView, l ), 0 );

	vec3 diffuse = colour * cosTheta;

	fragColor = vec4( lightColour * diffuse, 1 );
}
