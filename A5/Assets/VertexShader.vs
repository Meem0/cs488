#version 330

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 lightPosition;

in vec3 position;
in vec3 normal;
in vec2 texCoord;

out vec3 fragPositionView;
out vec3 lightPositionView;
out vec3 normalView;
out vec2 fragTexCoord;

void main() {
	gl_Position = P * V * M * vec4(position, 1.0);

	fragPositionView = (V * M * vec4(position, 1.0)).xyz;
	lightPositionView = (V * M * vec4(lightPosition, 1.0)).xyz;
	normalView = normalize(mat3(V * M) * normal);

	fragTexCoord = texCoord;
}
