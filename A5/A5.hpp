#pragma once

#define _USE_MATH_DEFINES

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"

#include <glm/glm.hpp>

class A5 : public CS488Window {
public:
	A5();
	virtual ~A5();

protected:
	virtual void init() override;
	virtual void appLogic() override;
	virtual void guiLogic() override;
	virtual void draw() override;
	virtual void cleanup() override;

	//-- Virtual callback methods
	virtual bool cursorEnterWindowEvent(int entered) override;
	virtual bool mouseMoveEvent(double xPos, double yPos) override;
	virtual bool mouseButtonInputEvent(int button, int actions, int mods) override;
	virtual bool mouseScrollEvent(double xOffSet, double yOffSet) override;
	virtual bool windowResizeEvent(int width, int height) override;
	virtual bool keyInputEvent(int key, int action, int mods) override;

private:
	const static int NumMouseButtons = 3;
	bool m_mouseButtonPressed[NumMouseButtons];
	glm::vec2 m_mousePos;

	void initGeom();

	// Fields related to the shader and uniforms.
	ShaderProgram m_shader;
	GLint m_uniformP; // Uniform location for Projection matrix.
	GLint m_uniformV; // Uniform location for View matrix.
	GLint m_uniformM; // Uniform location for Model matrix.
	GLint m_uniformColour;

	GLuint m_vaoPlane; // Vertex Array Object
	GLuint m_vboPlane; // Vertex Buffer Object

	glm::mat4 m_projMat;
	glm::mat4 m_viewMat;
};