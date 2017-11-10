#include "A5.hpp"

#include "cs488-framework/GlErrorCheck.hpp"
#include "cs488-framework/MathUtils.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;
using namespace std;

//----------------------------------------------------------------------------------------
// Constructor
A5::A5()
	: m_mouseButtonPressed{false, false, false}
	, m_mousePos(0, 0)
{
}

//----------------------------------------------------------------------------------------
// Destructor
A5::~A5()
{
}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void A5::init()
{
	// Set the background colour.
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	// Build the shader
	m_shader.generateProgramObject();
	m_shader.attachVertexShader(
		getAssetFilePath("VertexShader.vs").c_str());
	m_shader.attachFragmentShader(
		getAssetFilePath("FragmentShader.fs").c_str());
	m_shader.link();

	// Set up the uniforms
	m_uniformP = m_shader.getUniformLocation("P");
	m_uniformV = m_shader.getUniformLocation("V");
	m_uniformM = m_shader.getUniformLocation("M");
	m_uniformColour = m_shader.getUniformLocation("colour");

	initGeom();

	// Set up initial view and projection matrices (need to do this here,
	// since it depends on the GLFW window being set up correctly).
	m_cameraPos = vec3(0, 1.0f, 4.0f);
	m_cameraAngle = vec2(degreesToRadians(90.0f), 0);

	/*m_viewMat = glm::lookAt(
		m_cameraPos,
		m_cameraPos + vec3(0.0f, 0.0f, -1.0f),
		vec3(0.0f, 1.0f, 0.0f));*/

	m_projMat = perspective(
		radians(45.0f),
		float(m_framebufferWidth) / float(m_framebufferHeight),
		1.0f, 1000.0f);
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A5::appLogic()
{
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void A5::guiLogic()
{
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A5::draw()
{
	// Create a global transformation for the model (centre it).
	mat4 M;

	m_shader.enable();
	glEnable(GL_DEPTH_TEST);

	glUniformMatrix4fv(m_uniformP, 1, GL_FALSE, value_ptr(m_projMat));
	glUniformMatrix4fv(m_uniformV, 1, GL_FALSE, value_ptr(m_viewMat));
	glUniformMatrix4fv(m_uniformM, 1, GL_FALSE, value_ptr(M));

	// Just draw the grid for now.
	glBindVertexArray(m_vaoPlane);
	glUniform3f(m_uniformColour, 0.5f, 0.7f, 0.5f);
	glDrawArrays(GL_TRIANGLES, 0, 2 * 3);

	m_shader.disable();

	// Restore defaults
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void A5::cleanup()
{

}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool A5::cursorEnterWindowEvent (
		int entered
) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse cursor movement events.
 */
bool A5::mouseMoveEvent (
		double xPos,
		double yPos
) {
	bool eventHandled(false);

	vec2 mousePos(
		static_cast<float>(xPos),
		static_cast<float>(yPos)
	);

	float sensitivityX = degreesToRadians(540.0f) / m_windowWidth;
	float sensitivityY = degreesToRadians(540.0f) / m_windowHeight;

	vec2 angleDelta(
		sensitivityX * (mousePos.x - m_mousePos.x),
		sensitivityY * (mousePos.y - m_mousePos.y)
	);

	m_cameraAngle += angleDelta;

	//mat4 view = glm::lookAt(m_cameraPos, m_cameraPos + vec3(0, 0, -1.0f), vec3(0, 1.0f, 0));
	mat4 trans = glm::translate(mat4(), vec3() - m_cameraPos);
	mat4 rot = glm::rotate(mat4(), m_cameraAngle.x, vec3(0, 1.0f, 0));
	//m_viewMat = glm::rotate(m_viewMat, angleDelta.y, vec3(1.0f, 0, 0));

	m_viewMat = rot * trans;

	m_mousePos = mousePos;

	eventHandled = true;

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool A5::mouseButtonInputEvent (
		int button,
		int actions,
		int mods
) {
	bool eventHandled(false);

	if (button >= 0 && button < NumMouseButtons) {
		if (actions == GLFW_PRESS) {
			m_mouseButtonPressed[button] = true;

			eventHandled = true;
		}
		else if (actions == GLFW_RELEASE) {
			m_mouseButtonPressed[button] = false;

			eventHandled = true;
		}
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool A5::mouseScrollEvent (
		double xOffSet,
		double yOffSet
) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles window resize events.
 */
bool A5::windowResizeEvent (
		int width,
		int height
) {
	bool eventHandled(false);

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool A5::keyInputEvent (
		int key,
		int action,
		int mods
) {
	bool eventHandled(false);

	return eventHandled;
}

void A5::initGeom()
{
	const std::size_t numVerts = 6;
	vec3 verts[numVerts];
	verts[0] = vec3(-1.0f, 0, -1.0f);
	verts[1] = vec3(-1.0f, 0, 1.0f);
	verts[2] = vec3(1.0f, 0, 1.0f);
	verts[3] = vec3(-1.0f, 0, -1.0f);
	verts[4] = vec3(1.0f, 0, 1.0f);
	verts[5] = vec3(1.0f, 0, -1.0f);

	// Create the vertex array to record buffer assignments.
	glGenVertexArrays(1, &m_vaoPlane);
	glBindVertexArray(m_vaoPlane);

	// Create the plane vertex buffer
	glGenBuffers(1, &m_vboPlane);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboPlane);
	glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(vec3), verts, GL_STATIC_DRAW);

	// Specify the means of extracting the position values properly.
	GLint posAttrib = m_shader.getAttribLocation("position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Reset state
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	CHECK_GL_ERRORS;
}
