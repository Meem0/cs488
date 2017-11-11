#include "A5.hpp"

#include "cs488-framework/GlErrorCheck.hpp"
#include "cs488-framework/MathUtils.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui.h>

using namespace glm;
using namespace std;

//----------------------------------------------------------------------------------------
// Constructor
A5::A5()
	: m_mouseButtonPressed{ false, false, false }
	, m_mousePos(0, 0)
	, m_showMouse(false)
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

	double cursorX, cursorY;
	glfwGetCursorPos(m_window, &cursorX, &cursorY);
	m_mousePos.x = static_cast<float>(cursorX);
	m_mousePos.y = static_cast<float>(cursorY);

	// Set up initial view and projection matrices (need to do this here,
	// since it depends on the GLFW window being set up correctly).
	m_camera.moveTo(vec3(0, 2.0f, 4.0f));

	m_projMat = perspective(
		radians(45.0f),
		float(m_framebufferWidth) / float(m_framebufferHeight),
		0.1f, 1000.0f);
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A5::appLogic()
{
	m_camera.update(m_deltaTime);
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void A5::guiLogic()
{
	static bool debugWindow = true;

	ImGuiWindowFlags windowFlags(ImGuiWindowFlags_AlwaysAutoResize);
	float opacity(0.5f);

	ImGui::Begin("Debug Window", &debugWindow, ImVec2(100, 100), opacity, windowFlags);
	if (ImGui::Button("Quit Application")) {
		glfwSetWindowShouldClose(m_window, GL_TRUE);
	}
	ImGui::Text("Framerate: %.1f FPS", ImGui::GetIO().Framerate);
	ImGui::End();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A5::draw()
{
	// Create a global transformation for the model (centre it).
	mat4 M;

	mat4 V = m_camera.getViewMatrix();

	m_shader.enable();
	glEnable(GL_DEPTH_TEST);

	glUniformMatrix4fv(m_uniformP, 1, GL_FALSE, value_ptr(m_projMat));
	glUniformMatrix4fv(m_uniformV, 1, GL_FALSE, value_ptr(V));
	glUniformMatrix4fv(m_uniformM, 1, GL_FALSE, value_ptr(M));

	// draw the plane
	glBindVertexArray(m_vaoPlane);
	glUniform3f(m_uniformColour, 0.5f, 0.7f, 0.5f);
	glDrawArrays(GL_TRIANGLES, 0, 2 * 3);

	// draw the box
	glBindVertexArray(m_vaoBox);
	glUniform3f(m_uniformColour, 0.65f, 0.5f, 0.5f);
	glDrawArrays(GL_TRIANGLES, 0, 6 * 2 * 3);

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

	if (!m_showMouse) {
		vec2 sensitivity(
			degreesToRadians(90.0f) / m_windowWidth,
			degreesToRadians(90.0f) / m_windowHeight
		);

		vec2 angleDelta(
			sensitivity.x * (mousePos.x - m_mousePos.x),
			sensitivity.y * (mousePos.y - m_mousePos.y)
		);

		m_camera.rotate(angleDelta);
	}

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

	if (action == GLFW_PRESS || action == GLFW_RELEASE) {
		bool press = action == GLFW_PRESS;
		switch (key) {
		case GLFW_KEY_W:
			m_camera.setDirectionPressed(Camera::Direction::FORWARD, press);
			break;
		case GLFW_KEY_A:
			m_camera.setDirectionPressed(Camera::Direction::LEFT, press);
			break;
		case GLFW_KEY_S:
			m_camera.setDirectionPressed(Camera::Direction::BACKWARD, press);
			break;
		case GLFW_KEY_D:
			m_camera.setDirectionPressed(Camera::Direction::RIGHT, press);
			break;

		case GLFW_KEY_ESCAPE:
			if (press) {
				setShowMouse(!m_showMouse);
			}
			break;
		}
	}

	return eventHandled;
}

void A5::initGeom()
{
	const std::size_t planeNumVerts = 6;
	vec3 planeVerts[planeNumVerts];
	planeVerts[0] = vec3(-1.0f, 0, -1.0f);
	planeVerts[1] = vec3(-1.0f, 0, 1.0f);
	planeVerts[2] = vec3(1.0f, 0, 1.0f);
	planeVerts[3] = vec3(-1.0f, 0, -1.0f);
	planeVerts[4] = vec3(1.0f, 0, 1.0f);
	planeVerts[5] = vec3(1.0f, 0, -1.0f);

	// Create the vertex array to record buffer assignments.
	glGenVertexArrays(1, &m_vaoPlane);
	glBindVertexArray(m_vaoPlane);

	// Create the plane vertex buffer
	glGenBuffers(1, &m_vboPlane);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboPlane);
	glBufferData(GL_ARRAY_BUFFER, planeNumVerts * sizeof(vec3), planeVerts, GL_STATIC_DRAW);

	// Specify the means of extracting the position values properly.
	GLint posAttrib = m_shader.getAttribLocation("position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	const vec3 boxVerts[8] = {
		vec3(-0.8f, 0.3f, -0.8f), // 0 left-bottom-back
		vec3(-0.4f, 0.3f, -0.8f), // 1 right-bottom-back
		vec3(-0.8f, 0.7f, -0.8f), // 2 left-top-back
		vec3(-0.4f, 0.7f, -0.8f), // 3 right-top-back
		vec3(-0.8f, 0.3f, -0.4f), // 4 left-bottom-front
		vec3(-0.4f, 0.3f, -0.4f), // 5 right-bottom-front
		vec3(-0.8f, 0.7f, -0.4f), // 6 left-top-front
		vec3(-0.4f, 0.7f, -0.4f), // 7 right-top-front
	};

	const std::size_t boxNumVerts = 6 * 2 * 3;
	const int boxTris[boxNumVerts] = {
		2, 0, 4, // left
		2, 4, 6,
		3, 5, 1, // right
		3, 7, 5,
		5, 4, 0, // bottom
		5, 0, 1,
		3, 2, 6, // top
		3, 6, 7,
		3, 0, 2, // back
		3, 1, 0,
		7, 6, 4, // front
		7, 4, 5,
	};

	vec3 box[boxNumVerts];
	for (int i = 0; i < boxNumVerts; ++i) {
		box[i] = boxVerts[boxTris[i]];
	}

	// Create the vertex array to record buffer assignments.
	glGenVertexArrays(1, &m_vaoBox);
	glBindVertexArray(m_vaoBox);

	// Create the plane vertex buffer
	glGenBuffers(1, &m_vboBox);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboBox);
	glBufferData(GL_ARRAY_BUFFER, boxNumVerts * sizeof(vec3), box, GL_STATIC_DRAW);

	// Specify the means of extracting the position values properly.
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Reset state
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	CHECK_GL_ERRORS;
}

void A5::setShowMouse(bool showMouse)
{
	m_showMouse = showMouse;
	glfwSetInputMode(m_window, GLFW_CURSOR, m_showMouse ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

	m_showGui = showMouse;
}
