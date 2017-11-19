#include "A5.hpp"

#include "cs488-framework/GlErrorCheck.hpp"
#include "cs488-framework/MathUtils.hpp"
#include "cs488-framework/ShaderException.hpp"

#include "FastNoise.h"
#include "ObjFileDecoder.hpp"

#include <SOIL.h>

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
	, m_shininess(0)
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
		getAssetFilePath("VertexShader.vert").c_str());
	m_shader.attachFragmentShader(
		getAssetFilePath("FragmentShader.frag").c_str());
	m_shader.link();

	// Set up the uniforms
	m_uniformP = m_shader.getUniformLocation("P");
	m_uniformV = m_shader.getUniformLocation("V");
	m_uniformM = m_shader.getUniformLocation("M");
	m_uniformLightPosition = m_shader.getUniformLocation("lightPosition");
	m_uniformColour = m_shader.getUniformLocation("colour");
	m_uniformLightColour = m_shader.getUniformLocation("lightColour");
	m_uniformAmbientIntensity = m_shader.getUniformLocation("ambientIntensity");
	m_uniformSpecularCoeff = m_shader.getUniformLocation("specularCoeff");
	m_uniformShininess = m_shader.getUniformLocation("shininess");

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
	if (ImGui::SliderFloat("Ambient intensity", &m_ambientIntensity.x, 0, 1.0f)) {
		m_ambientIntensity.y = m_ambientIntensity.x;
		m_ambientIntensity.z = m_ambientIntensity.x;
	}
	/*if (ImGui::SliderFloat("Specular coeff.", &m_specularCoeff.x, 0, 2.0f)) {
		m_specularCoeff.y = m_specularCoeff.x;
		m_specularCoeff.z = m_specularCoeff.x;
	}
	ImGui::SliderFloat("Shininess", &m_shininess, 0, 100.0f);*/

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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUniformMatrix4fv(m_uniformP, 1, GL_FALSE, value_ptr(m_projMat));
	glUniformMatrix4fv(m_uniformV, 1, GL_FALSE, value_ptr(V));
	glUniformMatrix4fv(m_uniformM, 1, GL_FALSE, value_ptr(M));
	glUniform3fv(m_uniformLightPosition, 1, value_ptr(m_lightPosition));
	glUniform3fv(m_uniformLightColour, 1, value_ptr(lightColour));
	glUniform3fv(m_uniformAmbientIntensity, 1, value_ptr(m_ambientIntensity));
	glUniform3fv(m_uniformSpecularCoeff, 1, value_ptr(m_specularCoeff));
	glUniform1f(m_uniformShininess, m_shininess);

	if (m_wireframeMode) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// draw the terrain
	glBindVertexArray(m_vaoTerrain);
	glUniform3f(m_uniformColour, 1.0f, 1.0f, 1.0f);
	glDrawElements(GL_TRIANGLES, tilesIndexCount(m_terrainTileCount), GL_UNSIGNED_INT, nullptr);

	// draw the tree
	glBindVertexArray(m_vaoTree);
	glDrawElements(GL_TRIANGLES, m_treeIndexCount, GL_UNSIGNED_SHORT, nullptr);

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

	string treeObjectName;
	vector<vec3> treeVertices, treeNormals;
	vector<vec2> treeUVs;
	vector<FaceData> treeFaceData;
	ObjFileDecoder::decode(
		getAssetFilePath("treepineforest01.obj").c_str(),
		treeObjectName,
		treeVertices,
		treeNormals,
		treeUVs,
		treeFaceData
	);
	m_treeIndexCount = treeFaceData.size() * 3;

	for (auto& vert : treeVertices) {
		vert *= 0.01f;
	}

	// Create the vertex array to record buffer assignments.
	glGenVertexArrays(1, &m_vaoTree);
	glBindVertexArray(m_vaoTree);

	// Create the Tree vertex buffer
	GLuint vboTree;
	glGenBuffers(1, &vboTree);
	glBindBuffer(GL_ARRAY_BUFFER, vboTree);
	glBufferData(GL_ARRAY_BUFFER, treeVertices.size() * sizeof(vec3), treeVertices.data(), GL_STATIC_DRAW);

	// Specify the means of extracting the position values properly.
	GLint posAttrib = m_shader.getAttribLocation("position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the Tree normals vertex buffer
	GLuint vboTreeNormals;
	glGenBuffers(1, &vboTreeNormals);
	glBindBuffer(GL_ARRAY_BUFFER, vboTreeNormals);
	glBufferData(GL_ARRAY_BUFFER, treeNormals.size() * sizeof(vec3), treeNormals.data(), GL_STATIC_DRAW);

	// Specify the means of extracting the normal values properly.
	GLint normalAttrib = m_shader.getAttribLocation("normal");
	glEnableVertexAttribArray(normalAttrib);
	glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the Tree texture coordinates vertex buffer
	GLuint vboTreeUVs;
	glGenBuffers(1, &vboTreeUVs);
	glBindBuffer(GL_ARRAY_BUFFER, vboTreeUVs);
	glBufferData(GL_ARRAY_BUFFER, treeUVs.size() * sizeof(vec2), treeUVs.data(), GL_STATIC_DRAW);

	// Specify the means of extracting the textures values properly.
	GLint textureAttrib = m_shader.getAttribLocation("texCoord");
	glEnableVertexAttribArray(textureAttrib);
	glVertexAttribPointer(textureAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the tree element buffer
	glGenBuffers(1, &m_eboTree);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboTree);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, treeFaceData.size() * sizeof(FaceData), treeFaceData.data(), GL_STATIC_DRAW);


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

	// Specify the means of extracting the normal values properly.
	GLint normalAttrib = m_shader.getAttribLocation("normal");
	glEnableVertexAttribArray(normalAttrib);
	glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the terrain texture coordinates vertex buffer
	glGenBuffers(1, &m_vboTerrainTexCoords);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboTerrainTexCoords);
	glBufferData(GL_ARRAY_BUFFER, maxVertexCount * sizeof(vec2), nullptr, GL_STATIC_DRAW);

	// Specify the means of extracting the textures values properly.
	GLint textureAttrib = m_shader.getAttribLocation("texCoord");
	glEnableVertexAttribArray(textureAttrib);
	glVertexAttribPointer(textureAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the texture
	glGenTextures(1, &m_terrainTexture);
	glBindTexture(GL_TEXTURE_2D, m_terrainTexture);

	m_shader.enable();

	int width, height, channels;
	string texturePath = getAssetFilePath("vurt_PineAtlas04.dds");
	unsigned char* image = SOIL_load_image(texturePath.c_str(), &width, &height, &channels, SOIL_LOAD_AUTO);
	GLint format = channels == 4 ? GL_RGBA : GL_RGB;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	m_shader.disable();

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

	vector<vec2> terrainTexCoords(terrainVertexCount);
	for (std::size_t i = 0; i < terrainVertexCount; ++i) {
		terrainTexCoords[i].s = (terrainVertices[i].x + m_terrainWidth / 2.0f) / m_terrainWidth;
		terrainTexCoords[i].t = (terrainVertices[i].z + m_terrainWidth / 2.0f) / m_terrainWidth;
		//terrainTexCoords[i].s = terrainVertices[i].x;
		//terrainTexCoords[i].t = terrainVertices[i].z;
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

	glBindBuffer(GL_ARRAY_BUFFER, m_vboTerrainTexCoords);
	glBufferSubData(GL_ARRAY_BUFFER, 0, terrainVertexCount * sizeof(vec3), terrainTexCoords.data());
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
