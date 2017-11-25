#version 330

uniform vec3 colour;
uniform vec3 lightColour;
uniform vec3 ambientIntensity;
uniform vec3 specularCoeff;
uniform float shininess;
uniform sampler2D tex;

in vec3 fragPositionView;
in vec3 lightPositionView;
in vec3 normalView;
in vec2 fragTexCoord;

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

	vec4 texColour = texture(tex, fragTexCoord);
	vec4 diffuse = vec4(colour * texColour.xyz * cosTheta, texColour.a);

	if (diffuse.a < 0.5) {
		discard;
	}
	else {
		diffuse.a = 1.0;
	}

    // Direction from fragment to viewer (origin - fragPosition).
    vec3 v = normalize(-fragPositionView);
    vec3 specular = vec3(0.0);
    if (cosTheta > 0.0) {
		// Halfway vector.
		vec3 h = normalize(v + l);
        float n_dot_h = max(dot(normalView, h), 0.0);

        specular = specularCoeff * pow(n_dot_h, shininess);

		if (shininess >= 0) {
			specular = vec3(0);
		}
    }

	fragColor = vec4(ambientIntensity, 0) * texColour + vec4(lightColour, 1) * (diffuse + vec4(specular, 0));
}
