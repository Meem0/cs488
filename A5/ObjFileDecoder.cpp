#include "ObjFileDecoder.hpp"
using namespace glm;

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cassert>
#include <map>
using namespace std;

#include "cs488-framework/Exception.hpp"

void parseMaterialLib(
	const string& materialLibPath,
	std::vector<MaterialData>& materials,
	const map<string, std::size_t>& materialIndices
) {

	ifstream in(materialLibPath, std::ios::in);
	in.exceptions(std::ifstream::badbit);

	if (!in) {
		stringstream errorMessage;
		errorMessage << "Unable to open .mtl file " << materialLibPath
					 << " within method parseMaterialLib" << endl;

		throw Exception(errorMessage.str().c_str());
	}

	string currentLine;
	MaterialData* currentMaterial = nullptr;

	while (!in.eof()) {
		try {
			getline(in, currentLine);
		}
		catch (const ifstream::failure &e) {
			in.close();
			stringstream errorMessage;
			errorMessage << "Error calling getline() -- " << e.what() << endl;
			throw Exception(errorMessage.str());
		}
		if (currentLine.substr(0, 7) == "newmtl ") {
			string materialName;
			istringstream s(currentLine.substr(7));
			s >> materialName;

			auto itr = materialIndices.find(materialName);
			if (itr != materialIndices.end()) {
				currentMaterial = &materials[itr->second];
			}
		}
		else if (currentMaterial != nullptr && currentLine.substr(0, 7) == "map_Kd ") {
			istringstream s(currentLine.substr(7));
			s >> currentMaterial->diffuseMap;
		}
		else if (currentMaterial != nullptr && currentLine.substr(0, 5) == "bump ") {
			istringstream s(currentLine.substr(5));
			s >> currentMaterial->bumpMap;
		}
	}

	in.close();
}

//---------------------------------------------------------------------------------------
void ObjFileDecoder::decode(
	const char* objFilePath,
	std::string& objectName,
    std::vector<vec3>& positions,
    std::vector<vec3>& normals,
    std::vector<vec2>& uvCoords,
	std::vector<FaceData>& faces,
	std::vector<MaterialData>& materials
) {

	// Empty containers, and start fresh before inserting data from .obj file
	positions.clear();
	normals.clear();
	uvCoords.clear();
	faces.clear();
	materials.clear();

    ifstream in(objFilePath, std::ios::in);
    in.exceptions(std::ifstream::badbit);

    if (!in) {
        stringstream errorMessage;
        errorMessage << "Unable to open .obj file " << objFilePath
            << " within method ObjFileDecoder::decode" << endl;

        throw Exception(errorMessage.str().c_str());
    }

	// maps material names to indices of the groups vector
	map<string, std::size_t> materialIndices;
	string materialLibPath;

    string currentLine;
	unsigned short vn1, vn2, vn3, vt1, vt2, vt3;

	objectName = "";

    while (!in.eof()) {
        try {
            getline(in, currentLine);
        } catch (const ifstream::failure &e) {
            in.close();
            stringstream errorMessage;
            errorMessage << "Error calling getline() -- " << e.what() << endl;
            throw Exception(errorMessage.str());
        }
	    if (currentLine.substr(0, 2) == "o ") {
		    // Get entire line excluding first 2 chars.
		    istringstream s(currentLine.substr(2));
		    s >> objectName;
		}
		else if (currentLine.substr(0, 2) == "v ") {
            // Vertex data on this line.
            // Get entire line excluding first 2 chars.
            istringstream s(currentLine.substr(2));
            glm::vec3 vertex;
            s >> vertex.x;
            s >> vertex.y;
            s >> vertex.z;
            positions.push_back(vertex);
        }
		else if (currentLine.substr(0, 3) == "vn ") {
            // Normal data on this line.
            // Get entire line excluding first 2 chars.
            istringstream s(currentLine.substr(2));
            vec3 normal;
            s >> normal.x;
            s >> normal.y;
            s >> normal.z;
            normals.push_back(normal);
        }
		else if (currentLine.substr(0, 3) == "vt ") {
            // Texture coordinate data on this line.
            // Get entire line excluding first 2 chars.
            istringstream s(currentLine.substr(2));
            vec2 textureCoord;
            s >> textureCoord.s;
            s >> textureCoord.t;
			textureCoord.t = 1.0f - textureCoord.t;
            uvCoords.push_back(textureCoord);
        }
		else if (currentLine.substr(0, 2) == "f ") {
            // Face index data on this line.

            int index;

            // sscanf will return the number of matched index values it found
            // from the pattern.
            int numberOfIndexMatches = sscanf(currentLine.c_str(), "f %d/%d/%d",
                                              &index, &index, &index);

			FaceData faceData;

            if (numberOfIndexMatches == 3) {
                // Line contains indices of the pattern vertex/uv-cord/normal.
                sscanf(currentLine.c_str(), "f %hu/%hu/%hu %hu/%hu/%hu %hu/%hu/%hu",
                       &faceData.v1, &vt1, &vn1,
					   &faceData.v2, &vt2, &vn2,
					   &faceData.v3, &vt3, &vn3);
            } else {
                // Line contains indices of the pattern vertex//normal.
                sscanf(currentLine.c_str(), "f %hu//%hu %hu//%hu %hu//%hu",
					   &faceData.v1, &vn1,
					   &faceData.v2, &vn2,
					   &faceData.v3, &vn3);
            }

			assert(
				faceData.v1 == vt1 && faceData.v1 == vn1 &&
				faceData.v2 == vt2 && faceData.v2 == vn2 &&
				faceData.v3 == vt3 && faceData.v3 == vn3
			);

			--faceData.v1;
			--faceData.v2;
			--faceData.v3;

			faces.push_back(move(faceData));
		}
		else if (currentLine.substr(0, 7) == "mtllib ") {
			istringstream s(currentLine.substr(7));
			s >> materialLibPath;
		}
		else if (!materialLibPath.empty() && currentLine.substr(0, 7) == "usemtl ") {
			istringstream s(currentLine.substr(7));
			string materialName;
			s >> materialName;
			materialIndices.emplace(make_pair(materialName, materials.size()));

			materials.push_back(MaterialData{ faces.size(), "", "" });
		}
    }

    in.close();

	if (objectName.compare("") == 0) {
		// No 'o' object name tag defined in .obj file, so use the file name
		// minus the '.obj' ending as the objectName.
		const char * ptr = strrchr(objFilePath, '/');
		objectName.assign(ptr+1);
		size_t pos = objectName.find('.');
		objectName.resize(pos);
	}

	if (!materialLibPath.empty()) {
		string objFilePathStr(objFilePath);
		materialLibPath = objFilePathStr.substr(0, objFilePathStr.find_last_of("\\/") + 1) + materialLibPath;
		parseMaterialLib(materialLibPath, materials, materialIndices);
	}
}
