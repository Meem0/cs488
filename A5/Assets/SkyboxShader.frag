#version 330

uniform samplerCube cubemap;

in vec3 textureDir;

out vec4 fragColor;

void main() {
	fragColor = texture(cubemap, textureDir);
	//fragColor = vec4(0.86, 0.64, 0.2, 1.0);
}
