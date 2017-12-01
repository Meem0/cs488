#include "TestCube.hpp"

#include "Utility.hpp"

#include "cs488-framework/GlErrorCheck.hpp"

using namespace glm;
using namespace std;

TestCube::TestCube()
	: m_indexCount(0)
{
}

void TestCube::loadModel(const ShaderProgram& shader, const Mesh& mesh)
{
	m_mesh = mesh;
	m_indexCount = mesh.faceData.size() * 3;

	// Create the vertex array to record buffer assignments.
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	// Create the box vertex buffer
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

	// Create the box uTangents vertex buffer
	GLuint vboBoxUTangents;
	glGenBuffers(1, &vboBoxUTangents);
	glBindBuffer(GL_ARRAY_BUFFER, vboBoxUTangents);
	glBufferData(GL_ARRAY_BUFFER, mesh.uTangents.size() * sizeof(vec3), mesh.uTangents.data(), GL_STATIC_DRAW);

	// Specify the means of extracting the normal values properly.
	GLint uTangentAttrib = shader.getAttribLocation("uTangent");
	glEnableVertexAttribArray(uTangentAttrib);
	glVertexAttribPointer(uTangentAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the box normals vertex buffer
	GLuint vboBoxVTangents;
	glGenBuffers(1, &vboBoxVTangents);
	glBindBuffer(GL_ARRAY_BUFFER, vboBoxVTangents);
	glBufferData(GL_ARRAY_BUFFER, mesh.vTangents.size() * sizeof(vec3), mesh.vTangents.data(), GL_STATIC_DRAW);

	// Specify the means of extracting the normal values properly.
	GLint vTangentAttrib = shader.getAttribLocation("vTangent");
	glEnableVertexAttribArray(vTangentAttrib);
	glVertexAttribPointer(vTangentAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the box texture coordinates vertex buffer
	GLuint vboBoxUVs;
	glGenBuffers(1, &vboBoxUVs);
	glBindBuffer(GL_ARRAY_BUFFER, vboBoxUVs);
	glBufferData(GL_ARRAY_BUFFER, mesh.uvs.size() * sizeof(vec2), mesh.uvs.data(), GL_STATIC_DRAW);

	// Specify the means of extracting the textures values properly.
	GLint textureAttrib = shader.getAttribLocation("texCoord");
	glEnableVertexAttribArray(textureAttrib);
	glVertexAttribPointer(textureAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	if (mesh.groupData.size() > 0) {
		shader.enable();
		GLint textureUniform = shader.getUniformLocation("tex");
		glUniform1i(textureUniform, 0);
		GLint bumpMapUniform = shader.getUniformLocation("bump");
		glUniform1i(bumpMapUniform, 1);
		shader.disable();

		// Create the texture
		m_texture = Util::loadTexture(Util::getAssetFilePath(mesh.groupData[0].diffuseMap));

		// Create the bumpMap
		m_bumpMap = Util::loadTexture(Util::getAssetFilePath(mesh.groupData[0].bumpMap));
	}

	// Reset state
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	CHECK_GL_ERRORS;
}

void TestCube::draw()
{
	glBindVertexArray(m_vao);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_texture);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_bumpMap);

	glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_SHORT, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

void TestCube::debugDraw()
{
#if RENDER_DEBUG
	glColor3f(0, 0, 1.0f);
	glBegin(GL_LINES);

	for (int i = 0; i < m_mesh.positions.size(); ++i) {
		glm::vec3 p = m_mesh.positions[i];
		glVertex3fv(&p.x);
		glm::vec3 n = 0.1f * m_mesh.normals[i];
		p += n;
		glVertex3fv(&p.x);
	}

	glEnd();
#endif
}
