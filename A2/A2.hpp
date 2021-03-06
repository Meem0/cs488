#pragma once

#define _USE_MATH_DEFINES
#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <array>

// Set a global maximum number of vertices in order to pre-allocate VBO data
// in one shot, rather than reallocating each frame.
const GLsizei kMaxVertices = 1000;


// Convenience class for storing vertex data in CPU memory.
// Data should be copied over to GPU memory via VBO storage before rendering.
class VertexData {
public:
	VertexData();

	std::vector<glm::vec2> positions;
	std::vector<glm::vec3> colours;
	GLuint index;
	GLsizei numVertices;
};


class A2 : public CS488Window {
protected:
	enum class InteractionMode;
	typedef std::array<glm::vec2, 2> Line2D;
	const static Line2D NullLine;

public:
	A2();
	virtual ~A2();

protected:
	virtual void init() override;
	virtual void appLogic() override;
	virtual void guiLogic() override;
	virtual void draw() override;
	virtual void cleanup() override;

	virtual bool cursorEnterWindowEvent(int entered) override;
	virtual bool mouseMoveEvent(double xPos, double yPos) override;
	virtual bool mouseButtonInputEvent(int button, int actions, int mods) override;
	virtual bool mouseScrollEvent(double xOffSet, double yOffSet) override;
	virtual bool windowResizeEvent(int width, int height) override;
	virtual bool keyInputEvent(int key, int action, int mods) override;

	void createShaderProgram();
	void enableVertexAttribIndices();
	void generateVertexBuffers();
	void mapVboDataToVertexAttributeLocation();
	void uploadVertexDataToVbos();

	void initLineData();

	void setLineColour(const glm::vec3 & colour);

	void drawLine (
			const glm::vec2 & v0,
			const glm::vec2 & v1
	);

	void quit();
	void reset();

	void setInteractionMode(InteractionMode);

	void drawAlignedRect(glm::vec2 origin, glm::vec2 size);
	void drawClippedLine(glm::vec2 A, glm::vec2 B);
	glm::vec2 scaleToViewport(glm::vec2 point) const;
	void drawPerspectiveLine(glm::vec4 A, glm::vec4 B);

	void getDragBox(glm::vec2& origin, glm::vec2& size) const;

	ShaderProgram m_shader;

	GLuint m_vao;            // Vertex Array Object
	GLuint m_vbo_positions;  // Vertex Buffer Object
	GLuint m_vbo_colours;    // Vertex Buffer Object

	VertexData m_vertexData;

	glm::vec3 m_currentLineColour;

	glm::vec3 m_viewRotate;
	glm::vec3 m_viewTranslate;
	float m_fov;
	float m_nearPlaneDistance;
	float m_farPlaneDistance;

	glm::vec3 m_modelRotate;
	glm::vec3 m_modelTranslate;
	glm::vec3 m_modelScale;

	glm::mat4 m_modelMat;

	glm::vec2 m_viewportOrigin; // bottom-left, NDC
	glm::vec2 m_viewportSize; // NDC

	int m_interactionMode;

	bool m_leftMousePressed;
	bool m_middleMousePressed;
	bool m_rightMousePressed;
	bool m_dragViewport;

	double m_mouseXPos;
	double m_mouseYPos;
	double m_mouseXDragOrigin;
	double m_mouseYDragOrigin;
};
