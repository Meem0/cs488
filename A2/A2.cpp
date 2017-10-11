#include "A2.hpp"
#include "cs488-framework/GlErrorCheck.hpp"

#include <iostream>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
using namespace std;

#include <imgui/imgui.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
using namespace glm;

namespace {
	glm::mat4 translateMatrix3D(glm::vec3 t) {
		return glm::mat4(
			1.0f, 0, 0, 0,
			0, 1.0f, 0, 0,
			0, 0, 1.0f, 0,
			t.x, t.y, t.z, 1.0f
		);
	}

	float radToDeg(float rad) {
		return 2.0f * M_PI * rad / 360.0f;
	}

	glm::mat4 rotateMatrixX3D(float t) {
		t = radToDeg(t);
		return glm::mat4(
			1.0f, 0, 0, 0,
			0, cos(t), -sin(t), 0,
			0, sin(t), cos(t), 0,
			0, 0, 0, 1.0f
		);
	}

	glm::mat4 rotateMatrixY3D(float t) {
		t = radToDeg(t);
		return glm::mat4(
			cos(t), 0, sin(t), 0,
			0, 1.0f, 0, 0,
			-sin(t), 0, cos(t), 0,
			0, 0, 0, 1.0f
		);
	}

	glm::mat4 rotateMatrixZ3D(float t) {
		t = radToDeg(t);
		return glm::mat4(
			cos(t), sin(t), 0, 0,
			-sin(t), cos(t), 0, 0,
			0, 0, 1.0f, 0,
			0, 0, 0, 1.0f
		);
	}

	glm::mat4 rotateMatrix3D(glm::vec3 r) {
		return rotateMatrixX3D(r.x) * rotateMatrixY3D(r.y) * rotateMatrixZ3D(r.z);
	}

	glm::mat4 scaleMatrix3D(glm::vec3 s) {
		return glm::mat4(
			s.x, 0, 0, 0,
			0, s.y, 0, 0,
			0, 0, s.z, 0,
			0, 0, 0, 1.0f
		);
	}

	const static vector<char*> InteractionModeNames {
		"Rotate View",
		"Translate View",
		"Perspective",
		"Rotate Model",
		"Translate Model",
		"Scale Model",
		"Viewport",
	};

	template<typename TVec>
	bool clip(TVec& A, TVec& B, TVec P, TVec n) {
		float wecA = glm::dot(A - P, n);
		float wecB = glm::dot(B - P, n);

		if (wecA < 0 && wecB < 0) {
			return false;
		}
		if (!(wecA >= 0 && wecB >= 0)) {
			float t = wecA / (wecA - wecB);
			if (wecA < 0) {
				A = A + t * (B - A);
			}
			else {
				B = A + t * (B - A);
			}
		}

		return true;
	}
}

enum class A2::InteractionMode {
	RotateView,
	TranslateView,
	Perspective,
	RotateModel,
	TranslateModel,
	ScaleModel,
	Viewport,
};

const A2::Line2D A2::NullLine{
	vec2(numeric_limits<float>::min(), numeric_limits<float>::min()),
	vec2(numeric_limits<float>::min(), numeric_limits<float>::min())
};

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
	setInteractionMode(InteractionMode::RotateModel);

	m_viewRotate = glm::vec3();
	m_viewTranslate = glm::vec3();
	m_fov = 30.0f;
	m_nearPlaneDistance = 0.1f;
	m_farPlaneDistance = 100.0f;
	m_modelRotate = glm::vec3();
	m_modelTranslate = glm::vec3();
	m_modelScale = glm::vec3(1.0f, 1.0f, 1.0f);

	m_modelMat = glm::mat4();

	m_viewportOrigin = glm::vec2(-0.95f, -0.95f);
	m_viewportSize = glm::vec2(1.9f, 1.9f);
}

//----------------------------------------------------------------------------------------
void A2::setInteractionMode(InteractionMode interactionMode) {
	m_interactionMode = static_cast<int>(interactionMode);

	m_leftMousePressed = false;
	m_middleMousePressed = false;
	m_rightMousePressed = false;
	m_dragViewport = false;
}

//----------------------------------------------------------------------------------------
/*
* Get the current dragged box
*/
void A2::getDragBox(glm::vec2& origin, glm::vec2& size) const
{
	float xStart = std::min(m_mouseXPos, m_mouseXDragOrigin);
	float yStart = std::min(m_mouseYPos, m_mouseYDragOrigin);
	float xEnd = std::max(m_mouseXPos, m_mouseXDragOrigin);
	float yEnd = std::max(m_mouseYPos, m_mouseYDragOrigin);

	float convertWidth = 2.0f / static_cast<float>(m_windowWidth);
	float convertHeight = 2.0f / static_cast<float>(m_windowHeight);

	origin.x = convertWidth * xStart - 1.0f;
	origin.y = 1.0f - convertHeight * yEnd;

	size.x = convertWidth * (xEnd - xStart);
	size.y = convertHeight * (yEnd - yStart);
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
* Draw a rectangle aligned with the window
*/
void A2::drawAlignedRect(glm::vec2 origin, glm::vec2 size) {
	float x = origin.x;
	float y = origin.y;
	float w = size.x;
	float h = size.y;
	drawLine(vec2(x, y), vec2(x + w, y));
	drawLine(vec2(x + w, y), vec2(x + w, y + h));
	drawLine(vec2(x + w, y + h), vec2(x, y + h));
	drawLine(vec2(x, y + h), vec2(x, y));
}

//----------------------------------------------------------------------------------------
/*
* Draw a rectangle aligned with the window
*/
void A2::drawClippedLine(vec2 A, vec2 B) {
	array<vec2, 4> normals = {
		vec2(0, -1.0f), // top
		vec2(-1.0f, 0), // right
		vec2(0, 1.0f),  // bottom
		vec2(1.0f, 0)   // left
	};
	array<vec2, 4> points = {
		m_viewportOrigin + m_viewportSize,
		m_viewportOrigin + m_viewportSize,
		m_viewportOrigin,
		m_viewportOrigin
	};

	for (int i = 0; i < normals.size(); ++i) {
		vec2 P = points[i];
		vec2 n = normals[i];

		if (!clip(A, B, P, n)) {
			return;
		}
	}

	drawLine(A, B);
}

//----------------------------------------------------------------------------------------
vec2 A2::scaleToViewport(glm::vec2 point) const {
	point.x = m_viewportOrigin.x + ((point.x + 1.0f) / 2.0f) * m_viewportSize.x;
	point.y = m_viewportOrigin.y + ((point.y + 1.0f) / 2.0f) * m_viewportSize.y;
	return point;
}

//----------------------------------------------------------------------------------------
void A2::drawPerspectiveLine(vec4 A, vec4 B) {
	{
		const static float CameraDepth = -1.0f;
		vec3 P(0, 0, CameraDepth + m_nearPlaneDistance);
		vec3 n(0, 0, 1.0f);
		vec3 a(A.x, A.y, A.z);
		vec3 b(B.x, B.y, B.z);

		if (!clip(a, b, P, n)) {
			return;
		}

		A.x = a.x; A.y = a.y; A.z = a.z;
		B.x = b.x; B.y = b.y; B.z = b.z;
	}

	vec2 A2(A.x, A.y);
	vec2 B2(B.x, B.y);

	A2 = scaleToViewport(A2);
	B2 = scaleToViewport(B2);

	drawClippedLine(A2, B2);
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

	// Draw draggable viewport box:
	if (m_dragViewport)
	{
		setLineColour(vec3(0.0f, 0.0f, 0.0f));

		vec2 origin, size;
		getDragBox(origin, size);

		drawAlignedRect(origin, size);
	}

	// Draw outer square:
	{
		setLineColour(vec3(1.0f, 0.7f, 0.8f));
		drawAlignedRect(m_viewportOrigin, m_viewportSize);
	}

	// Draw inner square:
	setLineColour(vec3(0.2f, 1.0f, 1.0f));
	{
		typedef array<unsigned int, 2> LineIndex;
		const static unsigned int NumLines = 12;
		const static array<LineIndex, NumLines> LineIndices {
			LineIndex { 0, 1 },
			LineIndex { 1, 5 },
			LineIndex { 5, 4 },
			LineIndex { 4, 0 },
			LineIndex { 0, 2 },
			LineIndex { 1, 3 },
			LineIndex { 4, 6 },
			LineIndex { 5, 7 },
			LineIndex { 2, 3 },
			LineIndex { 3, 7 },
			LineIndex { 7, 6 },
			LineIndex { 6, 2 }
		};
		const static unsigned int NumPoints = 8;
		array<vec4, NumPoints> modelPoints = {
			vec4(-0.5f, -0.5f, -0.5f, 1.0f), // 0 left-bottom-back
			vec4(0.5f, -0.5f, -0.5f, 1.0f),  // 1 right-bottom-back
			vec4(-0.5f, 0.5f, -0.5f, 1.0f),  // 2 left-top-back
			vec4(0.5f, 0.5f, -0.5f, 1.0f),   // 3 right-top-back
			vec4(-0.5f, -0.5f, 0.5f, 1.0f),  // 4 left-bottom-front
			vec4(0.5f, -0.5f, 0.5f, 1.0f),   // 5 right-bottom-front
			vec4(-0.5f, 0.5f, 0.5f, 1.0f),   // 6 left-top-front
			vec4(0.5f, 0.5f, 0.5f, 1.0f),    // 7 right-top-front
		};

		mat4 modelScale = scaleMatrix3D(m_modelScale);
		mat4 viewRotate = rotateMatrix3D(m_viewRotate);
		mat4 viewTranslate = translateMatrix3D(m_viewTranslate);

		mat4 transform = viewTranslate * viewRotate * m_modelMat * modelScale;

		for (int i = 0; i < modelPoints.size(); ++i) {
			modelPoints[i] = transform * modelPoints[i];
		}

		for (const LineIndex& idx : LineIndices) {
			drawPerspectiveLine(
				modelPoints[idx[0]],
				modelPoints[idx[1]]
			);
		}
	}
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
		m_mouseYPos = yPos;

		double windowWidth = static_cast<double>(m_windowWidth);

		double rotateFactor = 360.0 / windowWidth;
		double rotateAmount = dx * rotateFactor;

		double translateFactor = 2.0 / windowWidth;
		double translateAmount = dx * translateFactor;

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
				m_viewTranslate.x += translateAmount;
			}
			if (m_middleMousePressed) {
				m_viewTranslate.y += translateAmount;
			}
			if (m_rightMousePressed) {
				m_viewTranslate.z += translateAmount;
			}
			break;
		case InteractionMode::Perspective:
			if (m_leftMousePressed) {
				const static float MinFOV = 5.0f;
				const static float MaxFOV = 160.0f;

				double fovFactor = ((MaxFOV - MinFOV) / 2.0f) / windowWidth;
				double fovAmount = dx * fovFactor;

				m_fov += fovAmount;
				m_fov = std::min(MaxFOV, m_fov);
				m_fov = std::max(MinFOV, m_fov);
			}
			if (m_middleMousePressed) {
				const static float MinNearPlane = 0.001f;
				const static float MaxNearPlane = 5.0f;

				double nearPlaneFactor = ((MaxNearPlane - MinNearPlane) / 5.0f) / windowWidth;
				double nearPlaneAmount = dx * nearPlaneFactor;

				m_nearPlaneDistance += nearPlaneAmount;
				m_nearPlaneDistance = std::min(MaxNearPlane, m_nearPlaneDistance);
				m_nearPlaneDistance = std::max(MinNearPlane, m_nearPlaneDistance);
			}
			if (m_rightMousePressed) {
				m_farPlaneDistance += dx;
			}
			break;
		case InteractionMode::RotateModel: {
			vec3 rotateDelta;
			if (m_leftMousePressed) {
				m_modelRotate.x += rotateAmount;
				rotateDelta.x = rotateAmount;
			}
			if (m_middleMousePressed) {
				m_modelRotate.y += rotateAmount;
				rotateDelta.y = rotateAmount;
			}
			if (m_rightMousePressed) {
				m_modelRotate.z += rotateAmount;
				rotateDelta.z = rotateAmount;
			}
			if (rotateDelta != vec3()) {
				m_modelMat = m_modelMat * rotateMatrix3D(rotateDelta);
			}
			break;
		}
		case InteractionMode::TranslateModel: {
			vec3 translateDelta;
			if (m_leftMousePressed) {
				m_modelTranslate.x += translateAmount;
				translateDelta.x = translateAmount;
			}
			if (m_middleMousePressed) {
				m_modelTranslate.y += translateAmount;
				translateDelta.y = translateAmount;
			}
			if (m_rightMousePressed) {
				m_modelTranslate.z += translateAmount;
				translateDelta.z = translateAmount;
			}
			if (translateDelta != vec3()) {
				m_modelMat = m_modelMat * translateMatrix3D(translateDelta);
			}
			break;
		}
		case InteractionMode::ScaleModel: {
			double scaleFactor = 2.0 / windowWidth;
			double scaleAmount = dx * scaleFactor;

			if (m_leftMousePressed) {
				m_modelScale.x += scaleAmount;
				m_modelScale.x = std::max(0.0f, m_modelScale.x);
			}
			if (m_middleMousePressed) {
				m_modelScale.y += scaleAmount;
				m_modelScale.y = std::max(0.0f, m_modelScale.y);
			}
			if (m_rightMousePressed) {
				m_modelScale.z += scaleAmount;
				m_modelScale.z = std::max(0.0f, m_modelScale.z);
			}
			break;
		}
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

		glfwGetCursorPos(m_window, &m_mouseXPos, &m_mouseYPos);

		if (button == GLFW_MOUSE_BUTTON_1) {
			if (actions == GLFW_PRESS) {
				m_leftMousePressed = true;

				if (m_interactionMode == static_cast<int>(InteractionMode::Viewport)) {
					m_mouseXDragOrigin = m_mouseXPos;
					m_mouseYDragOrigin = m_mouseYPos;
					m_dragViewport = true;
				}

				eventHandled = true;
			}
			else if (actions == GLFW_RELEASE) {
				m_leftMousePressed = false;

				if (m_interactionMode == static_cast<int>(InteractionMode::Viewport)) {
					getDragBox(m_viewportOrigin, m_viewportSize);

					m_dragViewport = false;
				}

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
			setInteractionMode(InteractionMode::RotateView);
			break;
		case GLFW_KEY_N:
			setInteractionMode(InteractionMode::TranslateView);
			break;
		case GLFW_KEY_P:
			setInteractionMode(InteractionMode::Perspective);
			break;
		case GLFW_KEY_R:
			setInteractionMode(InteractionMode::RotateModel);
			break;
		case GLFW_KEY_T:
			setInteractionMode(InteractionMode::TranslateModel);
			break;
		case GLFW_KEY_S:
			setInteractionMode(InteractionMode::ScaleModel);
			break;
		case GLFW_KEY_V:
			setInteractionMode(InteractionMode::Viewport);
			break;
		}
	}
	else if (action == GLFW_RELEASE) {
	}

	return eventHandled;
}
