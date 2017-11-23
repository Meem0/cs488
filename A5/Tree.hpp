#pragma once

#include "cs488-framework/OpenGLImport.hpp"

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct Mesh;
class ShaderProgram;

class Tree {
public:
	Tree();

	void loadModel(const ShaderProgram& shader, const Mesh& mesh);

	void setWorldPosition(const glm::vec3& pos);

	void draw();

private:
	GLint m_uniformM;
	glm::mat4 m_modelMat;

	struct GroupInfo {
		std::size_t indexCount;
		GLuint texture;
	};

	GLuint m_vao; // Vertex Array Object
	GLuint m_ebo; // Element Buffer Object
	std::vector<GroupInfo> m_meshGroups;
};
