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
void ObjFileDecoder::decode(const char* objFilePath, Mesh& mesh)
{
	// Empty containers, and start fresh before inserting data from .obj file
	mesh.positions.clear();
	mesh.normals.clear();
	mesh.uvs.clear();
	mesh.faceData.clear();
	mesh.groupData.clear();

	vector<char> buffer;
	Util::readFile(objFilePath, buffer);

	// maps material names to indices of the groups vector
	map<string, std::size_t> materialIndices;
	string materialLibPath;

	char* currentPos = buffer.data();
	unsigned short vn1, vn2, vn3, vt1, vt2, vt3;

	vector<unsigned short> fullFaceData;

	mesh.name = "";

    while (*currentPos != '\0') {
		if (strncmp(currentPos, "v ", 2) == 0) {
			currentPos += 2;

			glm::vec3 vertex;

			vertex.x = strtof(currentPos, &currentPos);
			vertex.y = strtof(currentPos, &currentPos);
			vertex.z = strtof(currentPos, &currentPos);

			mesh.positions.push_back(move(vertex));
        }
		else if (strncmp(currentPos, "vn ", 3) == 0) {
			currentPos += 3;

			vec3 normal;

			normal.x = strtof(currentPos, &currentPos);
			normal.y = strtof(currentPos, &currentPos);
			normal.z = strtof(currentPos, &currentPos);

			mesh.normals.push_back(move(normal));
        }
		else if (strncmp(currentPos, "vt ", 3) == 0) {
			currentPos += 3;

			vec2 uv;

			uv.s = strtof(currentPos, &currentPos);
			uv.t = strtof(currentPos, &currentPos);

			uv.t = 1.0f - uv.t;
			mesh.uvs.push_back(move(uv));
        }
		else if (strncmp(currentPos, "f ", 2) == 0) {
			currentPos += 2;

            // Face index data on this line.

			FaceData faceData;
			int count = 0;

#define get static_cast<unsigned short>(strtoul(currentPos, &currentPos, 0)) - 1

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

			if (faceData.v1 == vt1 && faceData.v1 == vn1 &&
				faceData.v2 == vt2 && faceData.v2 == vn2 &&
				faceData.v3 == vt3 && faceData.v3 == vn3) {
				mesh.faceData.push_back(move(faceData));
			}
			else {
				fullFaceData.push_back(faceData.v1);
				fullFaceData.push_back(vt1);
				fullFaceData.push_back(vn1);
				fullFaceData.push_back(faceData.v2);
				fullFaceData.push_back(vt2);
				fullFaceData.push_back(vn2);
				fullFaceData.push_back(faceData.v3);
				fullFaceData.push_back(vt3);
				fullFaceData.push_back(vn3);
			}
		}
		else if (strncmp(currentPos, "o ", 2) == 0) {
			currentPos += 2;

			char* endPos = currentPos;
			// advance until newline
			while (!isspace(*endPos)) {
				++endPos;
			}

			mesh.name = string(currentPos, endPos);
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
			materialIndices.emplace(make_pair(materialName, mesh.groupData.size()));
			mesh.groupData.push_back(MaterialData{ mesh.faceData.size(), "", "" });

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

	if (mesh.name.compare("") == 0) {
		// No 'o' object name tag defined in .obj file, so use the file name
		// minus the '.obj' ending as the objectName.
		const char * ptr = strrchr(objFilePath, '/');
		mesh.name.assign(ptr+1);
		size_t pos = mesh.name.find('.');
		mesh.name.resize(pos);
	}

	if (!materialLibPath.empty()) {
		string objFilePathStr(objFilePath);
		materialLibPath = objFilePathStr.substr(0, objFilePathStr.find_last_of("\\/") + 1) + materialLibPath;
		parseMaterialLib(materialLibPath, mesh.groupData, materialIndices);
	}


	if (!fullFaceData.empty()) {
		for (auto& faceData : mesh.faceData) {
			fullFaceData.push_back(faceData.v1);
			fullFaceData.push_back(faceData.v1);
			fullFaceData.push_back(faceData.v1);
			fullFaceData.push_back(faceData.v2);
			fullFaceData.push_back(faceData.v2);
			fullFaceData.push_back(faceData.v2);
			fullFaceData.push_back(faceData.v3);
			fullFaceData.push_back(faceData.v3);
			fullFaceData.push_back(faceData.v3);
		}
		mesh.faceData.clear();

		typedef unsigned long long PackedVertex;
		map<PackedVertex, unsigned short> m;
		vector<vec3> positions;
		vector<vec3> normals;
		vector<vec2> uvs;
		size_t faceDataPos = 0;
		for (size_t i = 0; i < fullFaceData.size(); i += 3) {
			unsigned short v = fullFaceData[i];
			unsigned short vt = fullFaceData[i + 1];
			unsigned short vn = fullFaceData[i + 2];

			unsigned short index = 0;

			PackedVertex pv =
				(static_cast<PackedVertex>(v) << (2 * 8 * sizeof(unsigned short))) |
				(static_cast<PackedVertex>(vt) << (8 * sizeof(unsigned short))) |
				static_cast<PackedVertex>(vn);
			auto itr = m.find(pv);
			if (itr == m.end()) {
				assert(positions.size() == normals.size() && positions.size() == uvs.size());

				index = static_cast<unsigned short>(positions.size());
				m.emplace(pv, index);

				positions.push_back(mesh.positions[v]);
				normals.push_back(mesh.normals[vn]);
				uvs.push_back(mesh.uvs[vt]);
			}
			else {
				index = itr->second;
			}

			if (faceDataPos == 0) {
				mesh.faceData.push_back(FaceData{ index, 0, 0 });
			}
			else if (faceDataPos == 1) {
				mesh.faceData.back().v2 = index;
			}
			else if (faceDataPos == 2) {
				mesh.faceData.back().v3 = index;
			}
			faceDataPos = (faceDataPos + 1) % 3;
		}

		swap(positions, mesh.positions);
		swap(normals, mesh.normals);
		swap(uvs, mesh.uvs);
	}

	Util::calculateTangents(mesh.positions, mesh.uvs, mesh.faceData, mesh.uTangents, mesh.vTangents);
}
