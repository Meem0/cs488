#include "TestCube.hpp"

#include "Utility.hpp"

using namespace glm;

TestCube::TestCube()
	: m_indexCount(0)
{
}

void TestCube::loadModel(const ShaderProgram& shader, const Mesh& mesh)
{
	m_indexCount = mesh.faceData.size() * 3;

	// Create the vertex array to record buffer assignments.
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	// Create the terrain vertex buffer
	GLuint vboBox;
	glGenBuffers(1, &vboBox);
	glBindBuffer(GL_ARRAY_BUFFER, vboBox);
	glBufferData(GL_ARRAY_BUFFER, mesh.positions.size() * sizeof(vec3), mesh.positions.data(), GL_STATIC_DRAW);

	// Create the box element buffer
	GLuint eboBox;
	glGenBuffers(1, &eboBox);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboBox);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.faceData.size() * sizeof(FaceData), mesh.faceData.data(), GL_STATIC_DRAW);

	// Specify the means of extracting the position values properly.
	GLint posAttrib = shader.getAttribLocation("position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the box normals vertex buffer
	GLuint vboBoxNormals;
	glGenBuffers(1, &vboBoxNormals);
	glBindBuffer(GL_ARRAY_BUFFER, vboBoxNormals);
	glBufferData(GL_ARRAY_BUFFER, mesh.normals.size() * sizeof(vec3), mesh.normals.data(), GL_STATIC_DRAW);

	// Specify the means of extracting the normal values properly.
	GLint normalAttrib = shader.getAttribLocation("normal");
	glEnableVertexAttribArray(normalAttrib);
	glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the box texture coordinates vertex buffer
	GLuint vboBoxUVs;
	glGenBuffers(1, &vboBoxUVs);
	glBindBuffer(GL_ARRAY_BUFFER, vboBoxUVs);
	glBufferData(GL_ARRAY_BUFFER, mesh.uvs.size() * sizeof(vec2), mesh.uvs.data(), GL_STATIC_DRAW);

	// Specify the means of extracting the textures values properly.
	GLint textureAttrib = shader.getAttribLocation("texCoord");
	glEnableVertexAttribArray(textureAttrib);
	glVertexAttribPointer(textureAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the texture
	m_texture = Util::loadTexture(Util::getAssetFilePath(mesh.groupData[0].diffuseMap));

	// Reset state
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void TestCube::draw()
{
	glBindVertexArray(m_vao);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
}
