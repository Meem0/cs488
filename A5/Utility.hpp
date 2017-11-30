#include "cs488-framework/OpenGLImport.hpp"

#include <vector>
#include <string>

namespace Util {
	void readFile(const std::string& path, std::vector<char>& buffer, bool binary = false);

	GLuint loadTexture(const std::string& texturePath);
	// order: right, left, top, bottom, back, front
	GLuint loadCubeMap(const std::vector<std::string>& texturePaths);

	//std::string getAssetFilePath(const char* filename);
	std::string getAssetFilePath(const std::string& filename);
	void setAssetFilePathBase(const std::string& path);

	void startDebugTimer(const std::string& message);
	void endDebugTimer(const std::string& message);
}
