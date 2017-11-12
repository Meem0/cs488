#version 330

uniform vec3 colour;
uniform vec3 lightColour;
uniform float lightIntensity;

in vec3 lightDirectionView;
in vec3 normalView;

out vec4 fragColor;

void main() {
	// Cosine of the angle between the normal and the light direction,
	// clamped above 0
	//  - light is at the vertical of the triangle -> 1
	//  - light is perpendicular to the triangle -> 0
	//  - light is behind the triangle -> 0
	float cosTheta = clamp( dot(normalView, lightDirectionView ), 0, 1 );
	//float cosTheta = 0.0;

	fragColor = vec4( colour * lightColour * lightIntensity * cosTheta, 1 );
}
