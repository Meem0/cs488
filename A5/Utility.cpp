#include "Utility.hpp"

#include "ObjFileDecoder.hpp"

#include <cassert>
#include <chrono>
#include <fstream>
#include <map>
#include <vector>

#define GLIML_NO_PVR 1
#define GLIML_NO_KTX 1
#include <gliml/gliml.h>

using namespace glm;
using namespace std;

namespace Util {

	void readFile(const std::string& path, vector<char>& buffer, bool binary) {
		ios_base::openmode flags = ios::in;
		if (binary) {
			flags |= ios::binary;
		}

		ifstream file(path, flags);
		file.seekg(0, ios::end);
		streamoff len = file.tellg();
		file.seekg(0, ios::beg);
		buffer.resize(static_cast<size_t>(len + 1));
		file.read(buffer.data(), len);
	}

	size_t loadTextureHelper(const string& texturePath, GLenum target) {
		// load file into memory (gliml doesn't have any file I/O functions)
		vector<char> buffer;
		readFile(texturePath, buffer, true);

		gliml::context c;
		c.enable_dxt(true);

		if (c.load(buffer.data(), buffer.size())) {
			assert(c.is_2d());
			assert(c.is_compressed());
			assert(c.num_faces() == 1);

			int faceIdx = 0;
			for (int mipIdx = 0; mipIdx < c.num_mipmaps(faceIdx); ++mipIdx) {
				glCompressedTexImage2D(
					target,
					mipIdx,
					c.image_internal_format(),
					c.image_width(faceIdx, mipIdx),
					c.image_height(faceIdx, mipIdx),
					0,
					c.image_size(faceIdx, mipIdx),
					c.image_data(faceIdx, mipIdx)
				);
			}

			return c.num_mipmaps(faceIdx);
		}
		else {
			return 0;
		}
	}

	map<string, GLuint> textureCache;

	GLuint loadTexture(const string& texturePath) {
		auto itr = textureCache.find(texturePath);
		if (itr != textureCache.end()) {
			return itr->second;
		}

		GLuint textureID = 0;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		size_t numMipmaps = loadTextureHelper(texturePath, GL_TEXTURE_2D);
		if (numMipmaps == 0) {
			glDeleteTextures(1, &textureID);
			return 0;
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numMipmaps - 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		textureCache.emplace(texturePath, textureID);

		return textureID;
	}

	GLuint loadCubeMap(const vector<string>& texturePaths) {
		GLuint textureID = 0;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

		for (int i = 0; i < texturePaths.size(); ++i) {
			if (!loadTextureHelper(texturePaths[i], GL_TEXTURE_CUBE_MAP_POSITIVE_X + i)) {
				glDeleteTextures(1, &textureID);
				return 0;
			}
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		return textureID;
	}

	void calculateTangents(
		const vector<vec3>& positions,
		const vector<vec2>& uvs,
		const vector<FaceData>& faces,
		vector<vec3>& uTangents,
		vector<vec3>& vTangents
	) {
		assert(positions.size() == uvs.size());

		uTangents.resize(positions.size());
		vTangents.resize(positions.size());

		for (const auto& face : faces) {
			const vec3& v1 = positions[face.v1];
			const vec3& v2 = positions[face.v2];
			const vec3& v3 = positions[face.v3];

			const vec2& vt1 = uvs[face.v1];
			const vec2& vt2 = uvs[face.v2];
			const vec2& vt3 = uvs[face.v3];

			vec3 deltaPos1 = v2 - v1;
			vec3 deltaPos2 = v3 - v1;

			vec2 deltaUV1 = vt2 - vt1;
			vec2 deltaUV2 = vt3 - vt1;

			float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
			vec3 uTangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y)*r;
			vec3 vTangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x)*r;

			uTangents[face.v1] = uTangent;
			uTangents[face.v2] = uTangent;
			uTangents[face.v3] = uTangent;

			vTangents[face.v1] = vTangent;
			vTangents[face.v2] = vTangent;
			vTangents[face.v3] = vTangent;
		}
	}

	string assetFilePathBase;

	string getAssetFilePath(const char* filename)
	{
		return assetFilePathBase + filename;
	}

	string getAssetFilePath(const string& filename)
	{
		return getAssetFilePath(filename.c_str());
	}

	void setAssetFilePathBase(const string& path)
	{
		assetFilePathBase = path;
	}

	map<string, chrono::steady_clock::time_point> debugTimerMap;
	bool debugTimerFirstTime = true;

	void startDebugTimer(const string& message)
	{
		debugTimerMap.emplace(message, chrono::steady_clock::now());
	}

	void endDebugTimer(const string& message)
	{
		using namespace chrono;

		steady_clock::time_point time = steady_clock::now();
		auto itr = debugTimerMap.find(message);

		duration<double> timeSpan = duration_cast<duration<double>>(time - itr->second);

		ios_base::openmode flags = ios::out;
		if (debugTimerFirstTime) {
			flags |= ios::app;
		}
		ofstream debugFile("log.txt", flags);
		debugFile << message << " " << timeSpan.count() << endl;
		debugTimerFirstTime = false;

		debugTimerMap.erase(itr);
	}
}
