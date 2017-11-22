#include "Tree.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "cs488-framework/ShaderProgram.hpp"
#include "cs488-framework/GlErrorCheck.hpp"

#include "ObjFileDecoder.hpp"
#include "Utility.hpp"

using namespace glm;
using namespace std;

Tree::Tree(ShaderProgram& shader)
	: m_shader(shader)
{
	m_uniformM = m_shader.getUniformLocation("M");
}

void Tree::loadModel(const string& objFileName) {
	string objectName;
	vector<vec3> vertices, normals;
	vector<vec2> uvs;
	vector<FaceData> faceData;
	vector<MaterialData> groupData;
	ObjFileDecoder::decode(
		Util::getAssetFilePath(objFileName).c_str(),
		objectName,
		vertices,
		normals,
		uvs,
		faceData,
		groupData
	);
	for (int i = 0; i < groupData.size(); ++i) {
		GLuint textureID;
		// Create the texture
		textureID = Util::loadTexture(Util::getAssetFilePath(groupData[i].diffuseMap.c_str()));

		size_t startIndex = groupData[i].startIndex;
		size_t endIndex = (i + 1 < groupData.size()) ? groupData[i + 1].startIndex : faceData.size();

		m_meshGroups.push_back({
			(endIndex - startIndex) * 3,
			textureID
		});
	}

	for (auto& vert : vertices) {
		vert *= 0.01f;
	}

	// Create the vertex array to record buffer assignments.
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	// Create the Tree vertex buffer
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3), vertices.data(), GL_STATIC_DRAW);

	// Specify the means of extracting the position values properly.
	GLint posAttrib = m_shader.getAttribLocation("position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the Tree normals vertex buffer
	GLuint vboNormals;
	glGenBuffers(1, &vboNormals);
	glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(vec3), normals.data(), GL_STATIC_DRAW);

	// Specify the means of extracting the normal values properly.
	GLint normalAttrib = m_shader.getAttribLocation("normal");
	glEnableVertexAttribArray(normalAttrib);
	glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the Tree texture coordinates vertex buffer
	GLuint vboUVs;
	glGenBuffers(1, &vboUVs);
	glBindBuffer(GL_ARRAY_BUFFER, vboUVs);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(vec2), uvs.data(), GL_STATIC_DRAW);

	// Specify the means of extracting the textures values properly.
	GLint textureAttrib = m_shader.getAttribLocation("texCoord");
	glEnableVertexAttribArray(textureAttrib);
	glVertexAttribPointer(textureAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the tree element buffer
	glGenBuffers(1, &m_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, faceData.size() * sizeof(FaceData), faceData.data(), GL_STATIC_DRAW);

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
