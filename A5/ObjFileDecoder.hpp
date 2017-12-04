#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>
#include <string>

template <typename T>
struct FaceDataT {
	T v1;
	T v2;
	T v3;
};

typedef FaceDataT<std::uint16_t> FaceData;
typedef FaceDataT<std::uint32_t> FaceData32;

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
	std::vector<glm::vec3> uTangents;
	std::vector<glm::vec3> vTangents;
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


