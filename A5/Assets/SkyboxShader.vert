#version 330

uniform mat4 P;
uniform mat4 V;

in vec3 position;

out vec3 textureDir;

void main() {
	textureDir = position;
	gl_Position = (P * V * vec4(position, 1.0)).xyww;
	//gl_Position = (P * V * vec4(position, 1.0));
}
