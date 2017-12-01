#version 330

uniform vec3 colour;
uniform vec3 lightColour;
uniform vec3 ambientIntensity;
uniform vec3 specularCoeff;
uniform float shininess;
uniform sampler2D tex;
uniform sampler2D bump;
uniform bool useBumpMap;

in vec3 lightDirectionTang;
in vec3 normalView;
in vec3 lightDirectionView;
in vec2 fragTexCoord;

out vec4 fragColor;

void main() {
	vec3 n;
	// direction from fragment to light
	vec3 l;
	if (useBumpMap) {
		n = normalize(texture(bump, fragTexCoord).rgb*2.0 - 1.0);
		l = normalize(lightDirectionTang);
	}
	else {
		n = normalize(normalView);
		l = normalize(lightDirectionView);
	}

	// Cosine of the angle between the normal and the light direction,
	// clamped above 0
	//  - light is at the vertical of the triangle -> 1
	//  - light is perpendicular to the triangle -> 0
	//  - light is behind the triangle -> 0
	float cosTheta = max( dot(n, l ), 0 );

	vec4 texColour = texture(tex, fragTexCoord);
	vec4 diffuse = vec4(colour * texColour.rgb * cosTheta, texColour.a);

	if (diffuse.a < 0.5) {
		discard;
	}
	else {
		diffuse.a = 1.0;
	}

	// make ambient light come straight-on
	l = vec3(0, 0, 1.0);
	cosTheta = max( dot(n, l), 0 );
	vec4 ambient = vec4(ambientIntensity * texColour.rgb * cosTheta, 0.0);

	fragColor = ambient + vec4(lightColour, 1) * diffuse;
}
