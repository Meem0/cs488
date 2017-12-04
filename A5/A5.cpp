#include "A5.hpp"

#include "cs488-framework/GlErrorCheck.hpp"
#include "cs488-framework/MathUtils.hpp"
#include "cs488-framework/ShaderException.hpp"

#include "FastNoise.h"
#include "ObjFileDecoder.hpp"
#include "Utility.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui.h>

#include <cassert>
#include <cmath>
#include <vector>

using namespace glm;
using namespace std;

namespace {
	uint32_t tilesVertexCount(uint32_t tileCount)
	{
		return (tileCount + 1) * (tileCount + 1);
	}

	uint32_t tilesIndexCount(uint32_t tileCount)
	{
		return tileCount * tileCount * 2 * 3;
	}

	float getTerrainHeight(const vector<vec3>& terrainVertices, uint32_t numTiles, float terrainWidth, vec2 pos) {
		vec2 posPercent = (pos + terrainWidth / 2.0f) / terrainWidth;
		posPercent = glm::clamp(posPercent, vec2(), vec2(1.0f, 1.0f));

		vec2 posIdxHigh = glm::ceil(posPercent * static_cast<float>(numTiles));
		vec2 posIdxLow = glm::floor(posPercent * static_cast<float>(numTiles));

		uint32_t rowU = static_cast<uint32_t>(posIdxLow.y);
		uint32_t rowD = static_cast<uint32_t>(posIdxHigh.y);
		uint32_t colL = static_cast<uint32_t>(posIdxLow.x);
		uint32_t colR = static_cast<uint32_t>(posIdxHigh.x);

		// vertices of the current square
		vec3 vul = terrainVertices[rowU * (numTiles + 1) + colL];
		const vec3& vdl = terrainVertices[rowD * (numTiles + 1) + colL];
		const vec3& vur = terrainVertices[rowU * (numTiles + 1) + colR];
		vec3 vdr = terrainVertices[rowD * (numTiles + 1) + colR];

		assert(vul.x - vdl.x < 0.0001f);
		assert(vul.z - vur.z < 0.0001f);
		assert(vdr.x - vur.x < 0.0001f);
		assert(vdr.z - vdl.z < 0.0001f);

		if (rowU != rowD || colL != colR) {
			// squared distances to hypotenuse vertices
			float dul = Util::distanceSquared(pos, vec2(vul.x, vul.z));
			float ddr = Util::distanceSquared(pos, vec2(vdr.x, vdr.z));

			vec3 mid = (vur + vdl) / 2.0f;
			vec3 vh = dul < ddr ? vul : vdr;
			vec3& vo = dul < ddr ? vdr : vul;
			vec3 d = mid - vh;
			// possible that tile is flat in x or z dimension, when we are off the grid
			float k = (abs(vo.x - vh.x) > abs(vo.z - vh.z)) ?
				(vo.x - vh.x) / d.x :
				(vo.z - vh.z) / d.z;
			vo.y = vh.y + k * d.y;
		}

		vec2 tilePosPercent(
			(pos.x - vul.x) / (vur.x - vul.x),
			(pos.y - vul.z) / (vdl.z - vul.z)
		);
		tilePosPercent = glm::clamp(tilePosPercent, vec2(), vec2(1.0f, 1.0f));

		float hl = vdl.y * tilePosPercent.y + vul.y * (1.0f - tilePosPercent.y);
		float hr = vdr.y * tilePosPercent.y + vur.y * (1.0f - tilePosPercent.y);
		float h = hr * tilePosPercent.x + hl * (1.0f - tilePosPercent.x);

		return h;
	}
}

//----------------------------------------------------------------------------------------
// Constructor
A5::A5()
	: m_mouseButtonPressed{ false, false, false }
	, m_mousePos(0, 0)
	, m_showMouse(false)
	, m_terrainTileCount(256)
	, m_terrainWidth(512.0f)
	, m_wireframeMode(false)
	, m_multisample(false)
	, m_useBumpMap(false)
	, m_heightScaleFactor(1.0f)
	, m_movementSpeed(4.0f)
	, m_lightIntensity(1.0f)
	, m_lightPosition(-100.0f, 50.0f, 0)
	, m_useShadows(false)
	, m_renderDebugQuad(false)
	, m_normalDebug(false)
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
	// Build the shader
	m_shader.generateProgramObject();
	m_shader.attachVertexShader(
		Util::getAssetFilePath("VertexShader.vert").c_str());
	m_shader.attachFragmentShader(
		Util::getAssetFilePath("FragmentShader.frag").c_str());
	m_shader.link();

	// Set up the uniforms
	m_uniformP = m_shader.getUniformLocation("P");
	m_uniformV = m_shader.getUniformLocation("V");
	m_uniformM = m_shader.getUniformLocation("M");
	m_uniformLightPosition = m_shader.getUniformLocation("lightPosition");
	m_uniformLightSpaceMatrix = m_shader.getUniformLocation("lightSpaceMatrix");
	m_uniformColour = m_shader.getUniformLocation("colour");
	m_uniformLightColour = m_shader.getUniformLocation("lightColour");
	m_uniformAmbientIntensity = m_shader.getUniformLocation("ambientIntensity");
	m_uniformUseBumpMap = m_shader.getUniformLocation("useBumpMap");
	m_uniformUseShadows = m_shader.getUniformLocation("useShadows");

	m_shader.enable();
	GLint textureUniform = m_shader.getUniformLocation("tex");
	glUniform1i(textureUniform, 0);
	textureUniform = m_shader.getUniformLocation("bump");
	glUniform1i(textureUniform, 1);
	textureUniform = m_shader.getUniformLocation("shadow");
	glUniform1i(textureUniform, 2);
	m_shader.disable();

	m_skyboxShader.generateProgramObject();
	m_skyboxShader.attachVertexShader(
		Util::getAssetFilePath("SkyboxShader.vert").c_str());
	m_skyboxShader.attachFragmentShader(
		Util::getAssetFilePath("SkyboxShader.frag").c_str());
	m_skyboxShader.link();

	m_uniformSkyboxP = m_skyboxShader.getUniformLocation("P");
	m_uniformSkyboxV = m_skyboxShader.getUniformLocation("V");


	m_depthShader.generateProgramObject();
	m_depthShader.attachVertexShader(
		Util::getAssetFilePath("DepthShader.vert").c_str());
	m_depthShader.attachFragmentShader(
		Util::getAssetFilePath("DepthShader.frag").c_str());
	m_depthShader.link();

	m_uniformDepthM = m_depthShader.getUniformLocation("M");
	m_uniformDepthLightSpaceMatrix = m_depthShader.getUniformLocation("lightSpaceMatrix");

	glGenFramebuffers(1, &m_depthBuffer);
	glGenTextures(1, &m_depthMap);

	glBindTexture(GL_TEXTURE_2D, m_depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, ShadowWidth, ShadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindFramebuffer(GL_FRAMEBUFFER, m_depthBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	CHECK_GL_ERRORS;


	m_debugQuadShader.generateProgramObject();
	m_debugQuadShader.attachVertexShader(
		Util::getAssetFilePath("DebugQuad.vert").c_str());
	m_debugQuadShader.attachFragmentShader(
		Util::getAssetFilePath("DebugQuad.frag").c_str());
	m_debugQuadShader.link();

	m_debugQuadShader.enable();
	GLint debugQuadTextureUniform = m_debugQuadShader.getUniformLocation("tex");
	glUniform1i(debugQuadTextureUniform, 0);
	m_debugQuadShader.disable();

	glGenVertexArrays(1, &m_vaoDebugQuad);
	glBindVertexArray(m_vaoDebugQuad);

	CHECK_GL_ERRORS;

	float debugQuadVertices[] = {
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		1.0f, -1.0f,  1.0f, 0.0f,
		1.0f,  1.0f,  1.0f, 1.0f
	};

	GLuint vboDebugQuad;
	glGenBuffers(1, &vboDebugQuad);
	glBindBuffer(GL_ARRAY_BUFFER, vboDebugQuad);
	glBufferData(GL_ARRAY_BUFFER, sizeof(debugQuadVertices), debugQuadVertices, GL_STATIC_DRAW);

	GLint posAttrib = m_debugQuadShader.getAttribLocation("position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

	GLint uvAttrib = m_debugQuadShader.getAttribLocation("texCoord");
	glEnableVertexAttribArray(uvAttrib);
	CHECK_GL_ERRORS;
	glVertexAttribPointer(uvAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	glBindVertexArray(0);

	CHECK_GL_ERRORS;


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
	vec3 posBefore = m_camera.getPosition();
	m_camera.update(m_deltaTime);

	if (m_camera.get2dWalkMode()) {
		vec3 pos = m_camera.getPosition();
		vec2 pos2(pos.x, pos.z);
		float radius = 0.5f;

		const Collider* collided = nullptr;
		for (const auto& collider : m_colliders) {
			if (collider.collide(pos2, radius)) {
				collided = &collider;
				break;
			}
		}

		if (collided != nullptr) {
			vec2 v = pos2 - collided->getPosition();
			vec2 vn = normalize(v);
			vec2 midPos = collided->getPosition() + normalize(v) * (radius + collided->getRadius() + 0.01f);
			pos.x = midPos.x;
			pos.z = midPos.y;
		}

		pos.y = getTerrainHeight(vec2(pos.x, pos.z));
		pos.y += 1.0f;

		m_camera.moveTo(pos);
	}
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

	vec3 pos(m_camera.getPosition());
	ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
	float height = getTerrainHeight(vec2(pos.x, pos.z));
	ImGui::Text("Height: %.2f", height);

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

	ImGui::End();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A5::draw()
{
	if (m_renderDebugQuad) {
		renderDepthBuffer();

		m_debugQuadShader.enable();

		glBindVertexArray(m_vaoDebugQuad);
		glBindTexture(GL_TEXTURE_2D, m_depthMap);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		CHECK_GL_ERRORS;

		m_debugQuadShader.disable();

		return;
	}

	// Set the background colour.
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	if (m_useShadows) {
		renderDepthBuffer();
	}

	// Create a global transformation for the model (centre it).
	mat4 M;

	mat4 V = m_camera.getViewMatrix();

	vec3 lightColour(1.0f, 1.0f, 1.0f);
	lightColour *= m_lightIntensity;

	// w = 0 for directional light
	vec4 lightPosition4(m_lightPosition, 0);

	m_shader.enable();
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUniformMatrix4fv(m_uniformP, 1, GL_FALSE, value_ptr(m_projMat));
	glUniformMatrix4fv(m_uniformV, 1, GL_FALSE, value_ptr(V));
	glUniformMatrix4fv(m_uniformM, 1, GL_FALSE, value_ptr(M));
	glUniform4fv(m_uniformLightPosition, 1, value_ptr(lightPosition4));
	glUniformMatrix4fv(m_uniformLightSpaceMatrix, 1, GL_FALSE, value_ptr(calculateLightSpaceMatrix()));
	glUniform3fv(m_uniformLightColour, 1, value_ptr(lightColour));
	glUniform3fv(m_uniformAmbientIntensity, 1, value_ptr(m_ambientIntensity));
	glUniform1i(m_uniformUseBumpMap, m_useBumpMap);
	glUniform1i(m_uniformUseShadows, m_useShadows);
	glUniform3f(m_uniformColour, 1.0f, 1.0f, 1.0f);

	if (m_wireframeMode) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (m_multisample) {
		glEnable(GL_MULTISAMPLE);
	}
	else {
		glDisable(GL_MULTISAMPLE);
	}

	drawTerrain();
	drawTrees(m_uniformM);

	m_shader.disable();

#if RENDER_DEBUG
	if (m_normalDebug) {
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(value_ptr(m_projMat));
		glMatrixMode(GL_MODELVIEW);
		glm::mat4 MV = V * M;
		glLoadMatrixf(value_ptr(MV));

		glColor3f(0, 0, 1.0f);
		glBegin(GL_LINES);

		for (size_t i = 0; i < m_terrainVertices.size(); ++i) {
			glm::vec3 p = m_terrainVertices[i];
			glVertex3fv(&p.x);
			glm::vec3 n = m_terrainNormals[i];
			p += n;
			glVertex3fv(&p.x);
		}

		glEnd();

		for (auto& collider : m_colliders) {
			collider.debugDraw();
		}
	}
#endif


	m_skyboxShader.enable();

	glDepthFunc(GL_LEQUAL);
	glCullFace(GL_FRONT);

	mat4 skyboxV = mat4(mat3(V));
	glUniformMatrix4fv(m_uniformSkyboxP, 1, GL_FALSE, value_ptr(m_projMat));
	glUniformMatrix4fv(m_uniformSkyboxV, 1, GL_FALSE, value_ptr(skyboxV));

	glBindVertexArray(m_vaoSkybox);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxCubemap);
	glDrawElements(GL_TRIANGLES, m_skyboxIndexCount, GL_UNSIGNED_SHORT, nullptr);

	glDepthFunc(GL_LESS);
	glCullFace(GL_BACK);

	m_skyboxShader.disable();


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

		case GLFW_KEY_C:
			if (press) {
				m_camera.set2dWalkMode(!m_camera.get2dWalkMode());
			}
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

		case GLFW_KEY_M:
			if (press) {
				m_multisample = !m_multisample;
			}
			break;

		case GLFW_KEY_B:
			if (press) {
				m_useBumpMap = !m_useBumpMap;
			}
			break;

		case GLFW_KEY_H:
			if (press) {
				m_renderDebugQuad = !m_renderDebugQuad;
			}
			break;

		case GLFW_KEY_G:
			if (press) {
				m_useShadows = !m_useShadows;
			}
			break;

		case GLFW_KEY_N:
			if (press) {
				m_normalDebug = !m_normalDebug;
			}
			break;
		}
	}

	return eventHandled;
}

void A5::initGeom()
{
	allocateTerrain();


	vector<string> skyboxTexturePaths {
		Util::getAssetFilePath("skybox/miramar_rt.dds"),
		Util::getAssetFilePath("skybox/miramar_lf.dds"),
		Util::getAssetFilePath("skybox/miramar_up.dds"),
		Util::getAssetFilePath("skybox/miramar_dn.dds"),
		Util::getAssetFilePath("skybox/miramar_bk.dds"),
		Util::getAssetFilePath("skybox/miramar_ft.dds")
	};
	m_skyboxCubemap = Util::loadCubeMap(skyboxTexturePaths);

	Mesh skyboxMesh;
	ObjFileDecoder::decode(Util::getAssetFilePath("skybox.obj").c_str(), skyboxMesh);
	m_skyboxIndexCount = skyboxMesh.faceData.size() * 3;

	glGenVertexArrays(1, &m_vaoSkybox);
	glBindVertexArray(m_vaoSkybox);

	GLuint vboSkybox;
	glGenBuffers(1, &vboSkybox);
	glBindBuffer(GL_ARRAY_BUFFER, vboSkybox);
	glBufferData(GL_ARRAY_BUFFER, skyboxMesh.positions.size() * sizeof(vec3), skyboxMesh.positions.data(), GL_STATIC_DRAW);

	GLuint eboSkybox;
	glGenBuffers(1, &eboSkybox);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboSkybox);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, skyboxMesh.faceData.size() * sizeof(FaceData), skyboxMesh.faceData.data(), GL_STATIC_DRAW);

	GLint posAttrib = m_skyboxShader.getAttribLocation("position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);


	static const std::size_t TreeGridSideCount = 8;
	const float TreeGridWidth = m_terrainWidth * 0.9f;
	vec3 positions[TreeGridSideCount * TreeGridSideCount];
	float gap = TreeGridWidth / static_cast<float>(TreeGridSideCount);
	for (int r = 0; r < TreeGridSideCount; ++r) {
		for (int c = 0; c < TreeGridSideCount; ++c) {
			positions[r * TreeGridSideCount + c] = vec3(
				r * gap - TreeGridWidth / 2.0f + gap / 2.0f,
				0.5f,
				c * gap - TreeGridWidth / 2.0f + gap / 2.0f
			);
		}
	}

	Mesh treeMesh;
	ObjFileDecoder::decode(Util::getAssetFilePath("treepineforest01.obj").c_str(), treeMesh);

	for (auto& vert : treeMesh.positions) {
		vert *= 0.01f;
	}

	for (const auto& pos : positions) {
		m_trees.emplace_back();
		m_trees.back().loadModel(m_shader, treeMesh);
		m_trees.back().setWorldPosition(pos);

		m_colliders.emplace_back(vec2(pos.x, pos.z), 2.0f);
	}

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
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxIndexCount * sizeof(unsigned short), nullptr, GL_STATIC_DRAW);

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

	// Create the terrain uTangents vertex buffer
	glGenBuffers(1, &m_vboTerrainuTangents);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboTerrainuTangents);
	glBufferData(GL_ARRAY_BUFFER, maxVertexCount * sizeof(vec3), nullptr, GL_STATIC_DRAW);

	// Specify the means of extracting the uTangent values properly.
	GLint uTangentAttrib = m_shader.getAttribLocation("uTangent");
	glEnableVertexAttribArray(uTangentAttrib);
	glVertexAttribPointer(uTangentAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the terrain vTangents vertex buffer
	glGenBuffers(1, &m_vboTerrainvTangents);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboTerrainvTangents);
	glBufferData(GL_ARRAY_BUFFER, maxVertexCount * sizeof(vec3), nullptr, GL_STATIC_DRAW);

	// Specify the means of extracting the vTangent values properly.
	GLint vTangentAttrib = m_shader.getAttribLocation("vTangent");
	glEnableVertexAttribArray(vTangentAttrib);
	glVertexAttribPointer(vTangentAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the terrain texture coordinates vertex buffer
	glGenBuffers(1, &m_vboTerrainTexCoords);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboTerrainTexCoords);
	glBufferData(GL_ARRAY_BUFFER, maxVertexCount * sizeof(vec2), nullptr, GL_STATIC_DRAW);

	// Specify the means of extracting the textures values properly.
	GLint textureAttrib = m_shader.getAttribLocation("texCoord");
	glEnableVertexAttribArray(textureAttrib);
	glVertexAttribPointer(textureAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the texture
	m_terrainTexture = Util::loadTexture(Util::getAssetFilePath("pineforest03.dds"));
	// Create the bumpmap
	m_terrainBumpMap = Util::loadTexture(Util::getAssetFilePath("pineforest03_n.dds"));

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

	createTerrainGeometry([noise](float x, float y) { return noise.GetNoise(x, y); });

	for (Tree& tree : m_trees) {
		vec3 pos = tree.getWorldPosition();
		pos.y = getTerrainHeight(vec2(pos.x, pos.z));
		tree.setWorldPosition(pos);
	}
}

void A5::createTerrainGeometry(function<float(float, float)> heightFunction)
{
	const uint32_t terrainVertexCount = tilesVertexCount(m_terrainTileCount);
	m_terrainVertices.resize(terrainVertexCount);

	const uint32_t n = m_terrainTileCount + 1;
	float tileWidth = m_terrainWidth / static_cast<float>(m_terrainTileCount);
	for (uint32_t i = 0; i < terrainVertexCount; ++i) {
		uint32_t row = i / n;
		uint32_t col = i % n;

		int centreIndexTimes2 = (n - 1);
		int colDistanceTimes2 = col * 2 - centreIndexTimes2;
		int rowDistanceTimes2 = row * 2 - centreIndexTimes2;

		float x = static_cast<float>(colDistanceTimes2) * tileWidth / 2.0f;
		float z = static_cast<float>(rowDistanceTimes2) * tileWidth / 2.0f;

		float noiseX = m_terrainWidth * (static_cast<float>(row) / static_cast<float>(n) - 0.5f);
		float noiseY = m_terrainWidth * (static_cast<float>(col) / static_cast<float>(n) - 0.5f);
		float y = m_heightScaleFactor * heightFunction(noiseX, noiseY);

		m_terrainVertices[i] = vec3(x, y, z);
	}

#if RENDER_DEBUG
	m_terrainNormals.resize(terrainVertexCount);
	vector<vec3>& terrainNormals = m_terrainNormals;
#else
	vector<vec3> terrainNormals(terrainVertexCount);
#endif
	for (uint32_t i = 0; i < terrainVertexCount; ++i) {
		uint32_t row = i / n;
		uint32_t col = i % n;

		uint32_t row2, col2;
		vec3 l, r, u, d, o;

		o = m_terrainVertices[row * n + col];

		row2 = row == 0 ? row : row - 1;
		u = m_terrainVertices[row2 * n + col] - o;

		row2 = row == n - 1 ? row : row + 1;
		d = m_terrainVertices[row2 * n + col] - o;

		col2 = col == 0 ? col : col - 1;
		l = m_terrainVertices[row * n + col2] - o;

		col2 = col == n - 1 ? col : col + 1;
		r = m_terrainVertices[row * n + col2] - o;

		vec3 v1 = (d == vec3() || r == vec3()) ? vec3() : glm::normalize(glm::cross(d, r));
		vec3 v2 = (r == vec3() || u == vec3()) ? vec3() : glm::normalize(glm::cross(r, u));
		vec3 v3 = (u == vec3() || l == vec3()) ? vec3() : glm::normalize(glm::cross(u, l));
		vec3 v4 = (l == vec3() || d == vec3()) ? vec3() : glm::normalize(glm::cross(l, d));

		vec3 normal = vec3(v1 + v2 + v3 + v4);

		terrainNormals[i] = glm::normalize(normal);
	}

	vector<vec2> terrainTexCoords(terrainVertexCount);
	for (uint32_t i = 0; i < terrainVertexCount; ++i) {
		terrainTexCoords[i].s = m_terrainVertices[i].x;
		terrainTexCoords[i].t = m_terrainVertices[i].z;
	}

	const uint32_t terrainIndexCount = tilesIndexCount(m_terrainTileCount);

	vector<FaceData32> terrainIndices(terrainIndexCount / 3);
	for (uint32_t tileIdx = 0; tileIdx < m_terrainTileCount * m_terrainTileCount; ++tileIdx) {
		uint32_t row = tileIdx / m_terrainTileCount;
		uint32_t col = tileIdx % m_terrainTileCount;

		uint32_t a, b, c, d;
		a = row * n + col;
		b = a + 1;
		c = b + n - 1;
		d = c + 1;

		uint32_t idx = tileIdx * 2;
		terrainIndices[idx].v1 = a;
		terrainIndices[idx].v2 = c;
		terrainIndices[idx].v3 = b;

		++idx;

		terrainIndices[idx].v1 = b;
		terrainIndices[idx].v2 = c;
		terrainIndices[idx].v3 = d;
	}

	vector<vec3> terrainuTangents;
	vector<vec3> terrainvTangents;
	Util::calculateTangents(m_terrainVertices, terrainTexCoords, terrainIndices, terrainuTangents, terrainvTangents);

	glBindBuffer(GL_ARRAY_BUFFER, m_vboTerrain);
	glBufferSubData(GL_ARRAY_BUFFER, 0, terrainVertexCount * sizeof(vec3), m_terrainVertices.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_vboTerrainNormals);
	glBufferSubData(GL_ARRAY_BUFFER, 0, terrainVertexCount * sizeof(vec3), terrainNormals.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_vboTerrainuTangents);
	glBufferSubData(GL_ARRAY_BUFFER, 0, terrainVertexCount * sizeof(vec3), terrainuTangents.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_vboTerrainvTangents);
	glBufferSubData(GL_ARRAY_BUFFER, 0, terrainVertexCount * sizeof(vec3), terrainvTangents.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_vboTerrainTexCoords);
	glBufferSubData(GL_ARRAY_BUFFER, 0, terrainVertexCount * sizeof(vec2), terrainTexCoords.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboTerrain);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, terrainIndices.size() * sizeof(FaceData32), terrainIndices.data());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

float A5::getTerrainHeight(glm::vec2 pos) const
{
	return ::getTerrainHeight(m_terrainVertices, m_terrainTileCount, m_terrainWidth, pos);
}

void A5::drawTerrain()
{
	// draw the terrain
	glBindVertexArray(m_vaoTerrain);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_terrainTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_terrainBumpMap);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_depthMap);

	glDrawElements(GL_TRIANGLES, tilesIndexCount(m_terrainTileCount), GL_UNSIGNED_INT, nullptr);

	glActiveTexture(GL_TEXTURE0);
}

void A5::drawTrees(GLint uniformM)
{
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_depthMap);
	glActiveTexture(GL_TEXTURE0);

	// draw the trees
	for (auto& tree : m_trees) {
		tree.draw(uniformM);
	}
}

glm::mat4 A5::calculateLightSpaceMatrix() const
{
	float nearPlane = 1.0f, farPlane = 1000.0f;
	glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, nearPlane, farPlane);
	glm::mat4 lightView = glm::lookAt(
		m_lightPosition,
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	return lightProjection * lightView;
}

void A5::renderDepthBuffer()
{
	glm::mat4 lightSpaceMatrix = calculateLightSpaceMatrix();

	glEnable(GL_DEPTH_TEST);

	m_depthShader.enable();

	glUniformMatrix4fv(m_uniformDepthLightSpaceMatrix, 1, GL_FALSE, value_ptr(lightSpaceMatrix));

	glViewport(0, 0, ShadowWidth, ShadowHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, m_depthBuffer);
	glClear(GL_DEPTH_BUFFER_BIT);
	drawTerrain();
	drawTrees(m_uniformDepthM);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	m_depthShader.disable();

	glViewport(0, 0, m_windowWidth, m_windowHeight);
}

void A5::setShowMouse(bool showMouse)
{
	m_showMouse = showMouse;
	glfwSetInputMode(m_window, GLFW_CURSOR, m_showMouse ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

	m_showGui = showMouse;
}
