#version 330

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 lightPosition;
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

void main() {
	gl_Position = P * V * M * vec4(position, 1.0);

	vec3 vertexPositionView = (V * M * vec4(position, 1.0)).xyz;
	vec3 lightPositionView = (V * M * vec4(lightPosition, 1.0)).xyz;
	lightDirectionView = lightPositionView - vertexPositionView;

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
}
