#version 330

uniform mat4 lightSpaceMatrix;
uniform mat4 M;

in vec3 position;

void main() {
	gl_Position = lightSpaceMatrix * M * vec4(position, 1.0);
}
