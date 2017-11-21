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

class ObjFileDecoder {
public:

	/**
	* Extracts vertex data from a Wavefront .obj file
	* If an object name parameter is present in the .obj file, objectName is set to that,
	* otherwise objectName is set to the name of the .obj file.
	*
	* [in] objFilePath - path to .obj file
	* [out] objectName - name given to object.
	* [out] positions - positions given in (x,y,z) model space.
	* [out] normals - normals given in (x,y,z) model space.
	* [out] uvCoords - texture coordinates in (u,v) parameter space.
	*/
    static void decode(
		    const char * objFilePath,
			std::string & objectName,
            std::vector<glm::vec3>& positions,
            std::vector<glm::vec3>& normals,
            std::vector<glm::vec2>& uvCoords,
			std::vector<FaceData>& faces,
			std::vector<MaterialData>& groups
    );
};

