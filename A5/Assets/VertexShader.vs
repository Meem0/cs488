#version 330

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 lightPositionWorld;

in vec3 position;

out vec3 lightDirectionView;
out vec3 normalView;

void main() {
	// Position of the vertex, in worldspace : M * position
	vec3 positionWorld = (M * vec4(position,1)).xyz;

	// Vector that goes from the vertex to the camera, in camera space.
	// In camera space, the camera is at the origin (0,0,0).
	vec3 positionView = (V * M * vec4(position,1)).xyz;
	vec3 eyeDirectionView = vec3(0,0,0) - positionView;

	// Vector that goes from the vertex to the light, in camera space. M is ommited because it's identity.
	vec3 lightPositionView = ( V * vec4(lightPositionWorld,1)).xyz;
	lightDirectionView = normalize(lightPositionView + eyeDirectionView);

	// Normal of the the vertex, in camera space
	// Only correct if ModelMatrix does not scale the model ! Use its inverse transpose if not.
	normalView = normalize(( V * M * vec4(0,1,0,0)).xyz);

	gl_Position = P * V * M * vec4(position, 1.0);
}
