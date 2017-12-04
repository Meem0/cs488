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

	glm::vec3 getWorldPosition() const;
	void setWorldPosition(const glm::vec3& pos);

	void draw(GLint uniformM);

private:
	glm::vec3 m_worldPosition;
	glm::mat4 m_modelMat;
	bool m_needToRecalculateModelMatrix;

	const glm::mat4& getModelMatrix();

	struct GroupInfo {
		std::size_t indexCount;
		GLuint diffuse;
		GLuint bumpmap;
	};

	GLuint m_vao; // Vertex Array Object
	GLuint m_ebo; // Element Buffer Object
	std::vector<GroupInfo> m_meshGroups;
};
