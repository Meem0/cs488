#include "A5.hpp"

#include "cs488-framework/GlErrorCheck.hpp"
#include "cs488-framework/MathUtils.hpp"
#include "cs488-framework/ShaderException.hpp"

#include "FastNoise.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui.h>

#include <cassert>
#include <vector>

using namespace glm;
using namespace std;

namespace {
	std::size_t tilesVertexCount(std::size_t tileCount)
	{
		return (tileCount + 1) * (tileCount + 1);
	}

	std::size_t tilesIndexCount(std::size_t tileCount)
	{
		return tileCount * tileCount * 2 * 3;
	}
}

//----------------------------------------------------------------------------------------
// Constructor
A5::A5()
	: m_mouseButtonPressed{ false, false, false }
	, m_mousePos(0, 0)
	, m_showMouse(false)
	, m_terrainTileCount(128)
	, m_terrainWidth(128.0f)
	, m_wireframeMode(false)
	, m_heightScaleFactor(1.0f)
	, m_movementSpeed(4.0f)
	, m_lightIntensity(1.0f)
	, m_lightPosition(-100.0f, 50.0f, 0)
{
	m_terrainTileCountSlider = static_cast<float>(m_terrainTileCount);
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
	m_uniformLightPosition = m_shader.getUniformLocation("lightPosition");
	m_uniformColour = m_shader.getUniformLocation("colour");
	m_uniformLightColour = m_shader.getUniformLocation("lightColour");

	initGeom();
	createTerrain();

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

	if (ImGui::SliderFloat("Movement speed", &m_movementSpeed, 0.5f, 100.0f, "%.3f", 2.0f)) {
		m_camera.setSpeed(m_movementSpeed);
	}

	if (ImGui::SliderFloat("Terrain width", &m_terrainWidth, 1.0f, 1024.0f, "%.3f", 4.0f)) {
		createTerrain();
	}
	if (ImGui::SliderFloat("Tile count", &m_terrainTileCountSlider, 1.0f, 1024.0f, "%.0f", 2.0f)) {
		m_terrainTileCountSlider = std::floor(m_terrainTileCountSlider);
		m_terrainTileCount = static_cast<std::size_t>(m_terrainTileCountSlider);
		createTerrain();
	}

	if (ImGui::SliderFloat("Height scale", &m_heightScaleFactor, 0.1f, 10.0f, "%.3f", 1.5f)) {
		createTerrain();
	}

	ImGui::SliderFloat3("Light position", &m_lightPosition.x, -200.0f, 200.0f);
	ImGui::SliderFloat("Light intensity", &m_lightIntensity, 0, 10.0f, "%.3f", 2.0f);

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

	vec3 lightColour(1.0f, 1.0f, 1.0f);
	lightColour *= m_lightIntensity;

	m_shader.enable();
	glEnable(GL_DEPTH_TEST);

	glUniformMatrix4fv(m_uniformP, 1, GL_FALSE, value_ptr(m_projMat));
	glUniformMatrix4fv(m_uniformV, 1, GL_FALSE, value_ptr(V));
	glUniformMatrix4fv(m_uniformM, 1, GL_FALSE, value_ptr(M));
	glUniform3fv(m_uniformLightPosition, 1, value_ptr(m_lightPosition));
	glUniform3fv(m_uniformLightColour, 1, value_ptr(lightColour));

	if (m_wireframeMode) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// draw the terrain
	glBindVertexArray(m_vaoTerrain);
	glUniform3f(m_uniformColour, 0.5f, 0.7f, 0.5f);
	glDrawElements(GL_TRIANGLES, tilesIndexCount(m_terrainTileCount), GL_UNSIGNED_INT, nullptr);

	// draw the box
	/*glBindVertexArray(m_vaoBox);
	glUniform3f(m_uniformColour, 0.65f, 0.5f, 0.5f);
	glDrawArrays(GL_TRIANGLES, 0, 6 * 2 * 3);*/

	m_shader.disable();

	// Restore defaults
	glBindVertexArray(0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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

		case GLFW_KEY_F:
			if (press) {
				m_wireframeMode = !m_wireframeMode;
			}
			break;
		}
	}

	return eventHandled;
}

void A5::initGeom()
{
	allocateTerrain();

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

	// Create the terrain vertex buffer
	glGenBuffers(1, &m_vboBox);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboBox);
	glBufferData(GL_ARRAY_BUFFER, boxNumVerts * sizeof(vec3), box, GL_STATIC_DRAW);

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

void A5::allocateTerrain()
{
	std::size_t maxVertexCount = tilesVertexCount(MaxTiles);
	std::size_t maxIndexCount = tilesIndexCount(MaxTiles);

	// Create the vertex array to record buffer assignments.
	glGenVertexArrays(1, &m_vaoTerrain);
	glBindVertexArray(m_vaoTerrain);

	// Create the terrain vertex buffer
	glGenBuffers(1, &m_vboTerrain);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboTerrain);
	glBufferData(GL_ARRAY_BUFFER, maxVertexCount * sizeof(vec3), nullptr, GL_STATIC_DRAW);

	// Create the terrain element buffer
	glGenBuffers(1, &m_eboTerrain);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboTerrain);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxIndexCount * sizeof(std::size_t), nullptr, GL_STATIC_DRAW);

	// Specify the means of extracting the position values properly.
	GLint posAttrib = m_shader.getAttribLocation("position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the terrain normals vertex buffer
	glGenBuffers(1, &m_vboTerrainNormals);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboTerrainNormals);
	glBufferData(GL_ARRAY_BUFFER, maxVertexCount * sizeof(vec3), nullptr, GL_STATIC_DRAW);

	// Specify the means of extracting the position values properly.
	GLint normalAttrib = m_shader.getAttribLocation("normal");
	glEnableVertexAttribArray(normalAttrib);
	glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Reset state
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void A5::createTerrain()
{
	assert(m_terrainTileCount <= MaxTiles);

	FastNoise noise;
	noise.SetSeed(20171111);
	noise.SetNoiseType(FastNoise::SimplexFractal);

	const std::size_t terrainVertexCount = tilesVertexCount(m_terrainTileCount);
	vector<vec3> terrainVertices(terrainVertexCount);

	const std::size_t n = m_terrainTileCount + 1;
	float tileWidth = m_terrainWidth / static_cast<float>(m_terrainTileCount);
	for (std::size_t i = 0; i < terrainVertexCount; ++i) {
		std::size_t row = i / n;
		std::size_t col = i % n;

		int centreIndexTimes2 = (n - 1);
		int colDistanceTimes2 = col * 2 - centreIndexTimes2;
		int rowDistanceTimes2 = row * 2 - centreIndexTimes2;

		float x = static_cast<float>(colDistanceTimes2) * tileWidth / 2.0f;
		float z = static_cast<float>(rowDistanceTimes2) * tileWidth / 2.0f;

		int noiseX = static_cast<int>(static_cast<float>(MaxTiles) * static_cast<float>(row) / static_cast<float>(n));
		int noiseY = static_cast<int>(static_cast<float>(MaxTiles) * static_cast<float>(col) / static_cast<float>(n));
		float y = m_heightScaleFactor * noise.GetNoise(noiseX, noiseY);

		terrainVertices[i] = vec3(x, y, z);
	}

	vector<vec3> terrainNormals(terrainVertexCount);
	for (std::size_t i = 0; i < terrainVertexCount; ++i) {
		std::size_t row = i / n;
		std::size_t col = i % n;

		std::size_t row2, col2;
		float hl, hr, hu, hd;

		row2 = row == 0 ? row : row - 1;
		hl = terrainVertices[row2 * n + col].y;

		row2 = row == n - 1 ? row : row + 1;
		hr = terrainVertices[row2 * n + col].y;

		col2 = col == 0 ? col : col - 1;
		hu = terrainVertices[row * n + col2].y;

		col2 = col == n - 1 ? col : col + 1;
		hd = terrainVertices[row * n + col2].y;

		vec3 normal(hl - hr, 2.0f, hd - hu);
		terrainNormals[i] = glm::normalize(normal);
	}

	const std::size_t terrainIndexCount = tilesIndexCount(m_terrainTileCount);
	vector<std::size_t> terrainIndices(terrainIndexCount);
	for (std::size_t tileIdx = 0; tileIdx < m_terrainTileCount * m_terrainTileCount; ++tileIdx) {
		std::size_t row = tileIdx / m_terrainTileCount;
		std::size_t col = tileIdx % m_terrainTileCount;

		std::size_t a, b, c, d;
		a = row * n + col;
		b = a + 1;
		c = b + n - 1;
		d = c + 1;

		std::size_t idx = tileIdx * 6;
		terrainIndices[idx++] = a;
		terrainIndices[idx++] = c;
		terrainIndices[idx++] = b;
		terrainIndices[idx++] = b;
		terrainIndices[idx++] = c;
		terrainIndices[idx++] = d;
	}

	glBindBuffer(GL_ARRAY_BUFFER, m_vboTerrain);
	glBufferSubData(GL_ARRAY_BUFFER, 0, terrainVertexCount * sizeof(vec3), terrainVertices.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_vboTerrainNormals);
	glBufferSubData(GL_ARRAY_BUFFER, 0, terrainVertexCount * sizeof(vec3), terrainNormals.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboTerrain);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, terrainIndexCount * sizeof(std::size_t), terrainIndices.data());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void A5::setShowMouse(bool showMouse)
{
	m_showMouse = showMouse;
	glfwSetInputMode(m_window, GLFW_CURSOR, m_showMouse ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

	m_showGui = showMouse;
}
