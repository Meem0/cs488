#version 330

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform mat4 lightSpaceMatrix;
uniform vec4 lightPosition;
uniform bool useBumpMap;

in vec3 position;
in vec3 normal;
in vec3 uTangent;
in vec3 vTangent;
in vec2 texCoord;

out vec3 lightDirectionTang;
out vec3 normalView;
out vec3 lightDirectionView;
out vec2 fragTexCoord;
out vec4 fragPosLightSpace;

void main() {
	gl_Position = P * V * M * vec4(position, 1.0);

	vec3 lightPositionView = (V * M * lightPosition).xyz;
	if (lightPosition.w == 0.0) {
		lightDirectionView = lightPositionView;
	}
	else {
		vec3 vertexPositionView = (V * M * vec4(position, 1.0)).xyz;
		lightDirectionView = lightPositionView - vertexPositionView;
	}

	normalView = normalize(mat3(V * M) * normal);

	if (useBumpMap) {
		vec3 uTangentView = normalize(mat3(V * M) * uTangent);
		vec3 vTangentView = normalize(mat3(V * M) * vTangent);

		mat3 tbn = transpose(mat3(
			uTangentView,
			vTangentView,
			normalView
		));

		lightDirectionTang = tbn * lightDirectionView;
	}

	fragTexCoord = texCoord;

	vec4 vertexWorldPos = M * vec4(position, 1.0);
    fragPosLightSpace = lightSpaceMatrix * vertexWorldPos;
}
