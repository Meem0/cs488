#version 330

uniform sampler2D tex;

in vec2 texCoord;

out vec4 FragColor;

void main() {
	vec3 texColour = texture(tex, texCoord).rgb;
	FragColor = vec4(texColour.rgb, 1.0);
}
