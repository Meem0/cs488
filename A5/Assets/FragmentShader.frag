#version 330

uniform vec3 colour;
uniform vec3 lightColour;
uniform vec3 ambientIntensity;
uniform vec3 specularCoeff;
uniform float shininess;
uniform sampler2D tex;
uniform sampler2D bump;
uniform sampler2D shadow;
uniform bool useBumpMap;
uniform bool useShadows;

in vec3 lightDirectionTang;
in vec3 normalView;
in vec3 lightDirectionView;
in vec2 fragTexCoord;
in vec4 fragPosLightSpace;

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
	vec3 diffuse = colour * cosTheta;

	vec4 texColour = texture(tex, fragTexCoord);

	if (texColour.a < 0.5) {
		discard;
	}
	else {
		texColour.a = 1.0;
	}

	// make ambient light come straight-on
	l = vec3(0, 0, 1.0);
	cosTheta = max( dot(n, l), 0 );
	vec3 ambient = ambientIntensity * cosTheta;

	float shadowValue = 0.0;
	if (useShadows) {
		vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
		projCoords = projCoords * 0.5 + 0.5;
		float closestDepth = texture(shadow, projCoords.xy).r; 
		float currentDepth = projCoords.z;
		shadowValue = currentDepth > closestDepth ? 1.0 : 0.0;
	}

	fragColor = vec4(ambient + (1.0 - shadowValue) * lightColour * diffuse, 1.0) * texColour;
}
