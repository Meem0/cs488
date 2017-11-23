#include "Tree.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "cs488-framework/ShaderProgram.hpp"
#include "cs488-framework/GlErrorCheck.hpp"

#include "ObjFileDecoder.hpp"
#include "Utility.hpp"

using namespace glm;
using namespace std;

Tree::Tree()
{
}

void Tree::loadModel(const ShaderProgram& shader, const Mesh& mesh) {
	m_uniformM = shader.getUniformLocation("M");

	for (size_t i = 0; i < mesh.groupData.size(); ++i) {
		GLuint textureID;
		// Create the texture
		textureID = Util::loadTexture(Util::getAssetFilePath(mesh.groupData[i].diffuseMap.c_str()));

		size_t startIndex = mesh.groupData[i].startIndex;
		size_t endIndex = (i + 1 < mesh.groupData.size()) ? mesh.groupData[i + 1].startIndex : mesh.faceData.size();

		m_meshGroups.push_back({
			(endIndex - startIndex) * 3,
			textureID
		});
	}

	// Create the vertex array to record buffer assignments.
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	// Create the Tree vertex buffer
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh.positions.size() * sizeof(vec3), mesh.positions.data(), GL_STATIC_DRAW);

	// Specify the means of extracting the position values properly.
	GLint posAttrib = shader.getAttribLocation("position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the Tree normals vertex buffer
	GLuint vboNormals;
	glGenBuffers(1, &vboNormals);
	glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
	glBufferData(GL_ARRAY_BUFFER, mesh.normals.size() * sizeof(vec3), mesh.normals.data(), GL_STATIC_DRAW);

	// Specify the means of extracting the normal values properly.
	GLint normalAttrib = shader.getAttribLocation("normal");
	glEnableVertexAttribArray(normalAttrib);
	glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the Tree texture coordinates vertex buffer
	GLuint vboUVs;
	glGenBuffers(1, &vboUVs);
	glBindBuffer(GL_ARRAY_BUFFER, vboUVs);
	glBufferData(GL_ARRAY_BUFFER, mesh.uvs.size() * sizeof(vec2), mesh.uvs.data(), GL_STATIC_DRAW);

	// Specify the means of extracting the textures values properly.
	GLint textureAttrib = shader.getAttribLocation("texCoord");
	glEnableVertexAttribArray(textureAttrib);
	glVertexAttribPointer(textureAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the tree element buffer
	glGenBuffers(1, &m_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER,
		mesh.faceData.size() * sizeof(FaceData),
		mesh.faceData.data(),
		GL_STATIC_DRAW
	);

	// Reset state
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	CHECK_GL_ERRORS;
}

void Tree::setWorldPosition(const vec3& position)
{
	m_modelMat = glm::translate(mat4(), position);
}

void Tree::draw()
{
	glUniformMatrix4fv(m_uniformM, 1, GL_FALSE, value_ptr(m_modelMat));

	glDisable(GL_CULL_FACE);

	size_t currentIndex = 0;
	glBindVertexArray(m_vao);
	for (const auto& group : m_meshGroups) {
		glBindTexture(GL_TEXTURE_2D, group.texture);
		glDrawElements(
			GL_TRIANGLES,
			group.indexCount,
			GL_UNSIGNED_SHORT,
			(GLvoid*)(currentIndex * sizeof(unsigned short))
		);

		currentIndex += group.indexCount;
	}

	glEnable(GL_CULL_FACE);
}
