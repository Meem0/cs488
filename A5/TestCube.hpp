#pragma once

#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"

#include "ObjFileDecoder.hpp"

class TestCube {
public:
	TestCube();
	void loadModel(const ShaderProgram& shader, const Mesh& mesh);
	void draw();
	void debugDraw();

private:
	GLuint m_vao; // Vertex Array Object
	GLuint m_texture;
	GLuint m_bumpMap;
	std::size_t m_indexCount;

	Mesh m_mesh;
};
