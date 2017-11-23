#include "ObjFileDecoder.hpp"
using namespace glm;

#include <fstream>
#include <cstring>
#include <cassert>
#include <map>
using namespace std;

#include "cs488-framework/Exception.hpp"

#include "Utility.hpp"

void parseMaterialLib(
	const string& materialLibPath,
	std::vector<MaterialData>& materials,
	const map<string, std::size_t>& materialIndices
) {
	vector<char> buffer;
	Util::readFile(materialLibPath, buffer);

	MaterialData* currentMaterial = nullptr;

	char* currentPos = buffer.data();

	while (*currentPos != '\0') {
		if (strncmp(currentPos, "newmtl ", 7) == 0) {
			currentPos += 7;

			string materialName;

			char* endPos = currentPos;
			// advance until newline
			while (!isspace(*endPos)) {
				++endPos;
			}

			materialName = string(currentPos, endPos);
			currentPos = endPos;

			auto itr = materialIndices.find(materialName);
			if (itr != materialIndices.end()) {
				currentMaterial = &materials[itr->second];
			}
		}
		else if (currentMaterial != nullptr && strncmp(currentPos, "map_Kd ", 7) == 0) {
			currentPos += 7;

			char* endPos = currentPos;
			// advance until newline
			while (!isspace(*endPos)) {
				++endPos;
			}

			currentMaterial->diffuseMap = string(currentPos, endPos);
			currentPos = endPos;
		}
		else if (currentMaterial != nullptr && strncmp(currentPos, "bump ", 5) == 0) {
			currentPos += 5;

			char* endPos = currentPos;
			// advance until newline
			while (!isspace(*endPos)) {
				++endPos;
			}

			currentMaterial->bumpMap = string(currentPos, endPos);
			currentPos = endPos;
		}

		// advance until newline
		while (!iscntrl(*currentPos)) {
			++currentPos;
		}

		// advance until next line with content
		while (isspace(*currentPos)) {
			++currentPos;
		}
	}
}

//---------------------------------------------------------------------------------------
void ObjFileDecoder::decode(
	const char* objFilePath,
	std::string& objectName,
    std::vector<vec3>& positions,
    std::vector<vec3>& normals,
    std::vector<vec2>& uvs,
	std::vector<FaceData>& faces,
	std::vector<MaterialData>& materials
) {

	// Empty containers, and start fresh before inserting data from .obj file
	positions.clear();
	normals.clear();
	uvs.clear();
	faces.clear();
	materials.clear();

	vector<char> buffer;
	Util::readFile(objFilePath, buffer);

	// maps material names to indices of the groups vector
	map<string, std::size_t> materialIndices;
	string materialLibPath;

	char* currentPos = buffer.data();
	unsigned short vn1, vn2, vn3, vt1, vt2, vt3;

	objectName = "";

    while (*currentPos != '\0') {
		if (strncmp(currentPos, "v ", 2) == 0) {
			currentPos += 2;

			glm::vec3 vertex;

			vertex.x = strtof(currentPos, &currentPos);
			vertex.y = strtof(currentPos, &currentPos);
			vertex.z = strtof(currentPos, &currentPos);

            positions.push_back(move(vertex));
        }
		else if (strncmp(currentPos, "vn ", 3) == 0) {
			currentPos += 3;

			vec3 normal;

			normal.x = strtof(currentPos, &currentPos);
			normal.y = strtof(currentPos, &currentPos);
			normal.z = strtof(currentPos, &currentPos);

			normals.push_back(move(normal));
        }
		else if (strncmp(currentPos, "vt ", 3) == 0) {
			currentPos += 3;

			vec2 uv;

			uv.s = strtof(currentPos, &currentPos);
			uv.t = strtof(currentPos, &currentPos);

			uv.t = 1.0f - uv.t;
			uvs.push_back(move(uv));
        }
		else if (strncmp(currentPos, "f ", 2) == 0) {
			currentPos += 2;

            // Face index data on this line.

			FaceData faceData;
			int count = 0;

#define get static_cast<unsigned short>(strtoul(currentPos, &currentPos, 0))

			faceData.v1 = get;
			assert(*currentPos == '/');
			++currentPos;
			vt1 = get;
			assert(*currentPos == '/');
			++currentPos;
			vn1 = get;

			faceData.v2 = get;
			assert(*currentPos == '/');
			++currentPos;
			vt2 = get;
			assert(*currentPos == '/');
			++currentPos;
			vn2 = get;

			faceData.v3 = get;
			assert(*currentPos == '/');
			++currentPos;
			vt3 = get;
			assert(*currentPos == '/');
			++currentPos;
			vn3 = get;

#undef get

			currentPos += count;

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
		else if (strncmp(currentPos, "o ", 2) == 0) {
			currentPos += 2;

			char* endPos = currentPos;
			// advance until newline
			while (!isspace(*endPos)) {
				++endPos;
			}

			objectName = string(currentPos, endPos);
			currentPos = endPos;
		}
		else if (strncmp(currentPos, "mtllib ", 7) == 0) {
			currentPos += 7;

			char* endPos = currentPos;
			// advance until newline
			while (!isspace(*endPos)) {
				++endPos;
			}

			materialLibPath = string(currentPos, endPos);
			currentPos = endPos;
		}
		else if (!materialLibPath.empty() && strncmp(currentPos, "usemtl ", 7) == 0) {
			currentPos += 7;

			char* endPos = currentPos;
			// advance until newline
			while (!isspace(*endPos)) {
				++endPos;
			}

			string materialName = string(currentPos, endPos);
			materialIndices.emplace(make_pair(materialName, materials.size()));
			materials.push_back(MaterialData{ faces.size(), "", "" });

			currentPos = endPos;
		}

		// advance until newline
		while (!iscntrl(*currentPos)) {
			++currentPos;
		}

		// advance until next line with content
		while (isspace(*currentPos)) {
			++currentPos;
		}
    }

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
