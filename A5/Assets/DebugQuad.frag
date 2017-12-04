#version 330

uniform sampler2D tex;

in vec2 fragTexCoord;

out vec4 FragColor;

void main() {
	vec3 texColour = texture(tex, fragTexCoord).rgb;
	FragColor = vec4(texColour.rgb, 1.0);
}
