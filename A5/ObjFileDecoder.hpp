#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>

struct FaceData {
	unsigned short v1;
	unsigned short v2;
	unsigned short v3;
};

struct MaterialData {
	std::size_t startIndex;
	std::string diffuseMap;
	std::string bumpMap;
};

struct Mesh {
	std::string name;
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	std::vector<FaceData> faceData;
	std::vector<MaterialData> groupData;
};

class ObjFileDecoder {
public:

	/**
	* Extracts vertex data from a Wavefront .obj file
	* If an object name parameter is present in the .obj file, objectName is set to that,
	* otherwise objectName is set to the name of the .obj file.
	*/
    static void decode(const char* objFilePath, Mesh& mesh);
};


