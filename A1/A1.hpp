#pragma once

#define _USE_MATH_DEFINES 1

#include <glm/glm.hpp>
#include <vector>
#include <array>

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"

#include "grid.hpp"

class A1 : public CS488Window {
public:
	A1();
	virtual ~A1();

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

private:
	void initGrid();
	void initBars();
	void initHighlight();

	void reset();

	void moveSelectedPosition(int deltaRow, int deltaCol);
	void setSelectedPosition(int row, int col);
	void setHeight(int row, int col, int height);
	
	void setGridHighlightPosition(int row, int col);

	// Fields related to grid logic.
	Grid m_grid;
	int m_gridSelectedRow;
	int m_gridSelectedCol;

	// Fields related to the shader and uniforms.
	ShaderProgram m_shader;
	GLint P_uni; // Uniform location for Projection matrix.
	GLint V_uni; // Uniform location for View matrix.
	GLint M_uni; // Uniform location for Model matrix.
	GLint col_uni;   // Uniform location for bar colour.

	// Fields related to grid geometry.
	GLuint m_grid_vao; // Vertex Array Object
	GLuint m_grid_vbo; // Vertex Buffer Object

	// Fields related to bar.
	GLuint m_bar_vao; // Vertex Array Object
	GLuint m_bar_vbo; // Vertex Buffer Object

	float* m_barCoords;

	// Fields related to grid highlight.
	GLuint m_highlight_vao; // Vertex Array Object
	GLuint m_highlight_vbo; // Vertex Buffer Object

	// Matrices controlling the camera and projection.
	glm::mat4 proj;
	glm::mat4 view;

	std::vector<std::array<float, 3>> m_colours;
	int m_currentColour;

	bool m_copyMode;
};
