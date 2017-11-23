#include "cs488-framework/OpenGLImport.hpp"

#include <string>

namespace Util {
	GLuint loadTexture(const std::string& texturePath);
	
	//std::string getAssetFilePath(const char* filename);
	std::string getAssetFilePath(const std::string& filename);
	void setAssetFilePathBase(const std::string& path);

	void startDebugTimer(const std::string& message);
	void endDebugTimer(const std::string& message);
}
