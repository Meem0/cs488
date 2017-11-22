#include "Utility.hpp"

#include <cassert>
#include <fstream>
#include <vector>

#define GLIML_NO_PVR 1
#define GLIML_NO_KTX 1
#include <gliml/gliml.h>

using namespace std;

namespace Util {
	GLuint loadTexture(const string& texturePath) {
		// load file into memory (gliml doesn't have any file I/O functions)
		vector<char> buffer;
		std::size_t size = 0;
		{
			ifstream file(texturePath, ios::in | ios::binary);
			file.seekg(0, ios::end);
			size = file.tellg();
			file.seekg(0, ios::beg);
			buffer.resize(size);
			file.read(buffer.data(), size);
		}

		gliml::context c;
		c.enable_dxt(true);

		GLuint textureID = 0;

		if (c.load(buffer.data(), size)) {
			assert(c.is_2d());
			assert(c.is_compressed());
			assert(c.num_faces() == 1);

			glGenTextures(1, &textureID);
			glBindTexture(GL_TEXTURE_2D, textureID);

			int faceIdx = 0;
			for (int mipIdx = 0; mipIdx < c.num_mipmaps(faceIdx); ++mipIdx) {
				glCompressedTexImage2D(
					GL_TEXTURE_2D,
					mipIdx,
					c.image_internal_format(),
					c.image_width(faceIdx, mipIdx),
					c.image_height(faceIdx, mipIdx),
					0,
					c.image_size(faceIdx, mipIdx),
					c.image_data(faceIdx, mipIdx)
				);
			}

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, c.num_mipmaps(faceIdx) - 1);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		return textureID;
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
}
