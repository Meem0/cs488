#include "A2.hpp"
#include "cs488-framework/GlErrorCheck.hpp"

#include <iostream>
#include <algorithm>
using namespace std;

#include <imgui/imgui.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
using namespace glm;

namespace {
	enum class InteractionMode {
		RotateView,
		TranslateView,
		Perspective,
		RotateModel,
		TranslateModel,
		ScaleModel,
		Viewport,
	};
	const static vector<char*> InteractionModeNames {
		"Rotate View",
		"Translate View",
		"Perspective",
		"Rotate Model",
		"Translate Model",
		"Scale Model",
		"Viewport",
	};
}

//----------------------------------------------------------------------------------------
// Constructor
VertexData::VertexData()
	: numVertices(0),
	  index(0)
{
	positions.resize(kMaxVertices);
	colours.resize(kMaxVertices);
}


//----------------------------------------------------------------------------------------
// Constructor
A2::A2()
	: m_currentLineColour(vec3(0.0f))
{

}

//----------------------------------------------------------------------------------------
// Destructor
A2::~A2()
{

}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void A2::init()
{
	// Set the background colour.
	glClearColor(0.3, 0.5, 0.7, 1.0);

	createShaderProgram();

	glGenVertexArrays(1, &m_vao);

	enableVertexAttribIndices();

	generateVertexBuffers();

	mapVboDataToVertexAttributeLocation();

	reset();
}

//----------------------------------------------------------------------------------------
void A2::quit() {
	glfwSetWindowShouldClose(m_window, GL_TRUE);
}

//----------------------------------------------------------------------------------------
void A2::reset() {
	m_interactionMode = static_cast<int>(InteractionMode::RotateModel);

	m_viewRotate = glm::vec3();
	m_viewTranslate = glm::vec3();
	m_fov = 30.0f;
	m_nearPlaneDistance = 0.1f;
	m_farPlaneDistance = 100.0f;
	m_modelRotate = glm::vec3();
	m_modelTranslate = glm::vec3();
	m_modelScale = glm::vec3(1.0f, 1.0f, 1.0f);

	m_leftMousePressed = false;
	m_middleMousePressed = false;
	m_rightMousePressed = false;
}

//----------------------------------------------------------------------------------------
void A2::createShaderProgram()
{
	m_shader.generateProgramObject();
	m_shader.attachVertexShader( getAssetFilePath("VertexShader.vs").c_str() );
	m_shader.attachFragmentShader( getAssetFilePath("FragmentShader.fs").c_str() );
	m_shader.link();
}

//----------------------------------------------------------------------------------------
void A2::enableVertexAttribIndices()
{
	glBindVertexArray(m_vao);

	// Enable the attribute index location for "position" when rendering.
	GLint positionAttribLocation = m_shader.getAttribLocation( "position" );
	glEnableVertexAttribArray(positionAttribLocation);

	// Enable the attribute index location for "colour" when rendering.
	GLint colourAttribLocation = m_shader.getAttribLocation( "colour" );
	glEnableVertexAttribArray(colourAttribLocation);

	// Restore defaults
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
void A2::generateVertexBuffers()
{
	// Generate a vertex buffer to store line vertex positions
	{
		glGenBuffers(1, &m_vbo_positions);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_positions);

		// Set to GL_DYNAMIC_DRAW because the data store will be modified frequently.
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * kMaxVertices, nullptr,
				GL_DYNAMIC_DRAW);


		// Unbind the target GL_ARRAY_BUFFER, now that we are finished using it.
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		CHECK_GL_ERRORS;
	}

	// Generate a vertex buffer to store line colors
	{
		glGenBuffers(1, &m_vbo_colours);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_colours);

		// Set to GL_DYNAMIC_DRAW because the data store will be modified frequently.
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * kMaxVertices, nullptr,
				GL_DYNAMIC_DRAW);


		// Unbind the target GL_ARRAY_BUFFER, now that we are finished using it.
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		CHECK_GL_ERRORS;
	}
}

//----------------------------------------------------------------------------------------
void A2::mapVboDataToVertexAttributeLocation()
{
	// Bind VAO in order to record the data mapping.
	glBindVertexArray(m_vao);

	// Tell GL how to map data from the vertex buffer "m_vbo_positions" into the
	// "position" vertex attribute index for any bound shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_positions);
	GLint positionAttribLocation = m_shader.getAttribLocation( "position" );
	glVertexAttribPointer(positionAttribLocation, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Tell GL how to map data from the vertex buffer "m_vbo_colours" into the
	// "colour" vertex attribute index for any bound shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_colours);
	GLint colorAttribLocation = m_shader.getAttribLocation( "colour" );
	glVertexAttribPointer(colorAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	//-- Unbind target, and restore default values:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

//---------------------------------------------------------------------------------------
void A2::initLineData()
{
	m_vertexData.numVertices = 0;
	m_vertexData.index = 0;
}

//---------------------------------------------------------------------------------------
void A2::setLineColour (
		const glm::vec3 & colour
) {
	m_currentLineColour = colour;
}

//---------------------------------------------------------------------------------------
void A2::drawLine(
		const glm::vec2 & v0,   // Line Start (NDC coordinate)
		const glm::vec2 & v1    // Line End (NDC coordinate)
) {

	m_vertexData.positions[m_vertexData.index] = v0;
	m_vertexData.colours[m_vertexData.index] = m_currentLineColour;
	++m_vertexData.index;
	m_vertexData.positions[m_vertexData.index] = v1;
	m_vertexData.colours[m_vertexData.index] = m_currentLineColour;
	++m_vertexData.index;

	m_vertexData.numVertices += 2;
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A2::appLogic()
{
	// Place per frame, application logic here ...

	// Call at the beginning of frame, before drawing lines:
	initLineData();

	// Draw outer square:
	setLineColour(vec3(1.0f, 0.7f, 0.8f));
	drawLine(vec2(-0.5f, -0.5f), vec2(0.5f, -0.5f));
	drawLine(vec2(0.5f, -0.5f), vec2(0.5f, 0.5f));
	drawLine(vec2(0.5f, 0.5f), vec2(-0.5f, 0.5f));
	drawLine(vec2(-0.5f, 0.5f), vec2(-0.5f, -0.5f));


	// Draw inner square:
	setLineColour(vec3(0.2f, 1.0f, 1.0f));
	drawLine(vec2(-0.25f, -0.25f), vec2(0.25f, -0.25f));
	drawLine(vec2(0.25f, -0.25f), vec2(0.25f, 0.25f));
	drawLine(vec2(0.25f, 0.25f), vec2(-0.25f, 0.25f));
	drawLine(vec2(-0.25f, 0.25f), vec2(-0.25f, -0.25f));
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void A2::guiLogic()
{
	static bool firstRun(true);
	if (firstRun) {
		ImGui::SetNextWindowPos(ImVec2(50, 50));
		firstRun = false;
	}

	static bool showDebugWindow(true);
	ImGuiWindowFlags windowFlags(ImGuiWindowFlags_AlwaysAutoResize);
	float opacity(0.5f);

	ImGui::Begin("Properties", &showDebugWindow, ImVec2(100,100), opacity,
			windowFlags);
	{
		// Create Button, and check if it was clicked:
		if (ImGui::Button("Reset")) {
			reset();
		}
		if (ImGui::Button("Quit Application")) {
			quit();
		}

		for (int i = 0; i < InteractionModeNames.size(); ++i) {
			ImGui::PushID(i);
			if (ImGui::RadioButton(InteractionModeNames[i], &m_interactionMode, i)) {
			}
			ImGui::PopID();
		}

		ImGui::Text("View Rotation: (%.1f, %.1f, %.1f)", m_viewRotate.x, m_viewRotate.y, m_viewRotate.z);
		ImGui::Text("View Translation: (%.1f, %.1f, %.1f)", m_viewTranslate.x, m_viewTranslate.y, m_viewTranslate.z);
		ImGui::Text("Field of View: %.1f°", m_fov);
		ImGui::Text("Near Plane: %.1f", m_nearPlaneDistance);
		ImGui::Text("Far Plane: %.1f", m_farPlaneDistance);
		ImGui::Text("Model Rotation: (%.1f, %.1f, %.1f)", m_modelRotate.x, m_modelRotate.y, m_modelRotate.z);
		ImGui::Text("Model Translation: (%.1f, %.1f, %.1f)", m_modelTranslate.x, m_modelTranslate.y, m_modelTranslate.z);
		ImGui::Text("Model Scale: (%.1f, %.1f, %.1f)", m_modelScale.x, m_modelScale.y, m_modelScale.z);

		ImGui::Text("Framerate: %.1f FPS", ImGui::GetIO().Framerate);

	}
	ImGui::End();
}

//----------------------------------------------------------------------------------------
void A2::uploadVertexDataToVbos() {

	//-- Copy vertex position data into VBO, m_vbo_positions:
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_positions);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec2) * m_vertexData.numVertices,
				m_vertexData.positions.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		CHECK_GL_ERRORS;
	}

	//-- Copy vertex colour data into VBO, m_vbo_colours:
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_colours);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * m_vertexData.numVertices,
				m_vertexData.colours.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		CHECK_GL_ERRORS;
	}
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A2::draw()
{
	uploadVertexDataToVbos();

	glBindVertexArray(m_vao);

	m_shader.enable();
		glDrawArrays(GL_LINES, 0, m_vertexData.numVertices);
	m_shader.disable();

	// Restore defaults
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void A2::cleanup()
{

}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool A2::cursorEnterWindowEvent (
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
bool A2::mouseMoveEvent (
		double xPos,
		double yPos
) {
	bool eventHandled(false);

	if (!ImGui::IsMouseHoveringAnyWindow()) {
		double xPosPrev = m_mouseXPos;
		double dx = xPos - xPosPrev;
		m_mouseXPos = xPos;

		double windowWidth = static_cast<double>(m_windowWidth);

		double rotateFactor = 360.0 / windowWidth;
		double rotateAmount = dx * rotateFactor;

		switch (static_cast<InteractionMode>(m_interactionMode)) {
		case InteractionMode::RotateView:
			if (m_leftMousePressed) {
				m_viewRotate.x += rotateAmount;
			}
			if (m_middleMousePressed) {
				m_viewRotate.y += rotateAmount;
			}
			if (m_rightMousePressed) {
				m_viewRotate.z += rotateAmount;
			}
			break;
		case InteractionMode::TranslateView:
			if (m_leftMousePressed) {
				m_viewTranslate.x += dx;
			}
			if (m_middleMousePressed) {
				m_viewTranslate.y += dx;
			}
			if (m_rightMousePressed) {
				m_viewTranslate.z += dx;
			}
			break;
		case InteractionMode::Perspective:
			if (m_leftMousePressed) {
				const static float MinFOV = 5.0f;
				const static float MaxFOV = 160.0f;

				double fovFactor = ((MaxFOV - MinFOV) / 2) / windowWidth;
				double fovAmount = dx * fovFactor;

				m_fov += fovAmount;
				m_fov = std::min(MaxFOV, m_fov);
				m_fov = std::max(MinFOV, m_fov);
			}
			if (m_middleMousePressed) {
				m_nearPlaneDistance += dx;
			}
			if (m_rightMousePressed) {
				m_farPlaneDistance += dx;
			}
			break;
		case InteractionMode::RotateModel:
			if (m_leftMousePressed) {
				m_modelRotate.x += rotateAmount;
			}
			if (m_middleMousePressed) {
				m_modelRotate.y += rotateAmount;
			}
			if (m_rightMousePressed) {
				m_modelRotate.z += rotateAmount;
			}
			break;
		case InteractionMode::TranslateModel:
			if (m_leftMousePressed) {
				m_modelTranslate.x += dx;
			}
			if (m_middleMousePressed) {
				m_modelTranslate.y += dx;
			}
			if (m_rightMousePressed) {
				m_modelTranslate.z += dx;
			}
			break;
		case InteractionMode::ScaleModel:
			double scaleFactor = 2.0 / windowWidth;
			double scaleAmount = dx * scaleFactor;

			if (m_leftMousePressed) {
				m_modelScale.x += scaleAmount;
			}
			if (m_middleMousePressed) {
				m_modelScale.y += scaleAmount;
			}
			if (m_rightMousePressed) {
				m_modelScale.z += scaleAmount;
			}
			break;
		}
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool A2::mouseButtonInputEvent (
		int button,
		int actions,
		int mods
) {
	bool eventHandled(false);

	if (!ImGui::IsMouseHoveringAnyWindow()) {

		double xPos, yPos;
		glfwGetCursorPos(m_window, &xPos, &yPos);
		m_mouseXPos = xPos;

		if (button == GLFW_MOUSE_BUTTON_1) {
			if (actions == GLFW_PRESS) {
				m_leftMousePressed = true;

				m_mouseXDragOrigin = xPos;
				m_mouseYDragOrigin = yPos;

				eventHandled = true;
			}
			else if (actions == GLFW_RELEASE) {
				m_leftMousePressed = false;
				eventHandled = true;
			}
		}
		if (button == GLFW_MOUSE_BUTTON_2) {
			if (actions == GLFW_PRESS) {
				m_rightMousePressed = true;
				eventHandled = true;
			}
			else if (actions == GLFW_RELEASE) {
				m_rightMousePressed = false;
				eventHandled = true;
			}
		}
		if (button == GLFW_MOUSE_BUTTON_3) {
			if (actions == GLFW_PRESS) {
				m_middleMousePressed = true;
				eventHandled = true;
			}
			else if (actions == GLFW_RELEASE) {
				m_middleMousePressed = false;
				eventHandled = true;
			}
		}
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool A2::mouseScrollEvent (
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
bool A2::windowResizeEvent (
		int width,
		int height
) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool A2::keyInputEvent (
		int key,
		int action,
		int mods
) {
	bool eventHandled(false);

	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_A:
			reset();
			break;
		case GLFW_KEY_Q:
			quit();
			break;
		case GLFW_KEY_O:
			m_interactionMode = static_cast<int>(InteractionMode::RotateView);
			break;
		case GLFW_KEY_N:
			m_interactionMode = static_cast<int>(InteractionMode::TranslateView);
			break;
		case GLFW_KEY_P:
			m_interactionMode = static_cast<int>(InteractionMode::Perspective);
			break;
		case GLFW_KEY_R:
			m_interactionMode = static_cast<int>(InteractionMode::RotateModel);
			break;
		case GLFW_KEY_T:
			m_interactionMode = static_cast<int>(InteractionMode::TranslateModel);
			break;
		case GLFW_KEY_S:
			m_interactionMode = static_cast<int>(InteractionMode::ScaleModel);
			break;
		case GLFW_KEY_V:
			m_interactionMode = static_cast<int>(InteractionMode::Viewport);
			break;
		}
	}
	else if (action == GLFW_RELEASE) {
	}

	return eventHandled;
}
