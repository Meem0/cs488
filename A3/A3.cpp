#include "A3.hpp"
#include "scene_lua.hpp"
using namespace std;

#include "cs488-framework/GlErrorCheck.hpp"
#include "cs488-framework/MathUtils.hpp"
#include "GeometryNode.hpp"
#include "JointNode.hpp"

#include <imgui/imgui.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <limits>

using namespace glm;

static bool show_gui = true;

const size_t CIRCLE_PTS = 48;

namespace Trackball {

	vec3 getNormalizedVec(float diameter, vec2 mousePos) {
		// new position normalized such that 1 is the radius of the trackball
		vec2 normalizedPos = (2.0f / diameter) * mousePos;
		float normalizedPosZ = (1.0f - normalizedPos.x * normalizedPos.x - normalizedPos.y * normalizedPos.y);

		/* If the Z component is less than 0, the mouse point
		* falls outside of the trackball which is interpreted
		* as rotation about the Z axis.
		*/
		if (normalizedPosZ < 0) {
			float fLength = sqrt(1.0f - normalizedPosZ);
			normalizedPosZ = 0;

			normalizedPos.x /= fLength;
			normalizedPos.y /= fLength;
		}
		else {
			normalizedPosZ = sqrt(normalizedPosZ);
		}

		return vec3(normalizedPos.x, normalizedPos.y, normalizedPosZ);
	}

	vec3 calculateRotationVector(vec2 mouseNew, vec2 mouseOld, float diameter) {
		vec3 newVec = getNormalizedVec(diameter, mouseNew);
		vec3 oldVec = getNormalizedVec(diameter, mouseOld);

		/* Generate rotation vector by calculating cross product:
		*
		* fOldVec x fNewVec.
		*
		* The rotation vector is the axis of rotation
		* and is non-unit length since the length of a crossproduct
		* is related to the angle between fOldVec and fNewVec which we need
		* in order to perform the rotation.
		*/

		return glm::cross(oldVec, newVec);
	}

	mat4 getTrackballRotate(vec2 mouseOld, vec2 mouseNew, vec2 windowSize)
	{
		/* vCalcRotVec expects new and old positions in relation to the center of the
		* trackball circle which is centered in the middle of the window and
		* half the smaller of nWinWidth or nWinHeight.
		*/
		float diameter = (windowSize.x < windowSize.y) ? windowSize.x * 0.5f : windowSize.y * 0.5f;
		vec2 center(windowSize.x / 2.0f, windowSize.y / 2.0f);
		vec2 oldMod = mouseOld - center;
		vec2 newMod = mouseNew - center;

		vec3 rotVec = calculateRotationVector(newMod, oldMod, diameter);

		/* Negate Y component since Y axis increases downwards
		* in screen space and upwards in OpenGL.
		*/
		rotVec.y *= -1.0f;

		float length = glm::length(rotVec);
		vec3 norm = glm::normalize(rotVec);
		mat4 newMat = glm::rotate(mat4(), length, norm);

		// Since all these matrices are meant to be loaded
		// into the OpenGL matrix stack, we need to transpose the
		// rotation matrix (since OpenGL wants rows stored
		// in columns)
		newMat = glm::transpose(newMat);
		return newMat;
	}
}

//----------------------------------------------------------------------------------------
// Constructor
A3::A3(const std::string & luaSceneFile)
	: m_luaSceneFile(luaSceneFile)
	, m_positionAttribLocation(0)
	, m_normalAttribLocation(0)
	, m_vao_meshData(0)
	, m_vbo_vertexPositions(0)
	, m_vbo_vertexNormals(0)
	, m_vao_arcCircle(0)
	, m_vbo_arcCircle(0)
	, m_renderSceneNode(*this)
	, m_drawCircle(false)
	, m_useZBuffer(true)
	, m_backfaceCulling(false)
	, m_frontfaceCulling(false)
	, m_jointMode(0)
	, m_mouseButtonPressed{false, false, false}
{
}

//----------------------------------------------------------------------------------------
// Destructor
A3::~A3()
{

}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void A3::init()
{
	// Set the background colour.
	glClearColor(0.35f, 0.35f, 0.35f, 1.0f);

	createShaderProgram();

	glGenVertexArrays(1, &m_vao_arcCircle);
	glGenVertexArrays(1, &m_vao_meshData);
	enableVertexShaderInputSlots();

	processLuaSceneFile(m_luaSceneFile);

	// Load and decode all .obj files at once here.  You may add additional .obj files to
	// this list in order to support rendering additional mesh types.  All vertex
	// positions, and normals will be extracted and stored within the MeshConsolidator
	// class.
	unique_ptr<MeshConsolidator> meshConsolidator (new MeshConsolidator{
			getAssetFilePath("cube.obj"),
			getAssetFilePath("sphere.obj"),
			getAssetFilePath("suzanne.obj")
	});


	// Acquire the BatchInfoMap from the MeshConsolidator.
	meshConsolidator->getBatchInfoMap(m_batchInfoMap);

	// Take all vertex data within the MeshConsolidator and upload it to VBOs on the GPU.
	uploadVertexDataToVbos(*meshConsolidator);

	mapVboDataToVertexShaderInputLocations();

	initPerspectiveMatrix();

	initViewMatrix();

	initLightSources();

	m_transformStack.clear();
	m_transformStack.push_back(glm::mat4());

	resetAll();

	// Exiting the current scope calls delete automatically on meshConsolidator freeing
	// all vertex data resources.  This is fine since we already copied this data to
	// VBOs on the GPU.  We have no use for storing vertex data on the CPU side beyond
	// this point.
}

//----------------------------------------------------------------------------------------
void A3::processLuaSceneFile(const std::string & filename) {
	// This version of the code treats the Lua file as an Asset,
	// so that you'd launch the program with just the filename
	// of a puppet in the Assets/ directory.
	// std::string assetFilePath = getAssetFilePath(filename.c_str());
	// m_rootNode = std::shared_ptr<SceneNode>(import_lua(assetFilePath));

	// This version of the code treats the main program argument
	// as a straightforward pathname.
	m_rootNode.reset(import_lua(filename));
	if (!m_rootNode) {
		std::cerr << "Could not open " << filename << std::endl;
	}
}

//----------------------------------------------------------------------------------------
void A3::createShaderProgram()
{
	m_shader.generateProgramObject();
	m_shader.attachVertexShader( getAssetFilePath("VertexShader.vs").c_str() );
	m_shader.attachFragmentShader( getAssetFilePath("FragmentShader.fs").c_str() );
	m_shader.link();

	m_shader_arcCircle.generateProgramObject();
	m_shader_arcCircle.attachVertexShader( getAssetFilePath("arc_VertexShader.vs").c_str() );
	m_shader_arcCircle.attachFragmentShader( getAssetFilePath("arc_FragmentShader.fs").c_str() );
	m_shader_arcCircle.link();
}

//----------------------------------------------------------------------------------------
void A3::enableVertexShaderInputSlots()
{
	//-- Enable input slots for m_vao_meshData:
	{
		glBindVertexArray(m_vao_meshData);

		// Enable the vertex shader attribute location for "position" when rendering.
		m_positionAttribLocation = m_shader.getAttribLocation("position");
		glEnableVertexAttribArray(m_positionAttribLocation);

		// Enable the vertex shader attribute location for "normal" when rendering.
		m_normalAttribLocation = m_shader.getAttribLocation("normal");
		glEnableVertexAttribArray(m_normalAttribLocation);

		CHECK_GL_ERRORS;
	}


	//-- Enable input slots for m_vao_arcCircle:
	{
		glBindVertexArray(m_vao_arcCircle);

		// Enable the vertex shader attribute location for "position" when rendering.
		m_arc_positionAttribLocation = m_shader_arcCircle.getAttribLocation("position");
		glEnableVertexAttribArray(m_arc_positionAttribLocation);

		CHECK_GL_ERRORS;
	}

	// Restore defaults
	glBindVertexArray(0);
}

//----------------------------------------------------------------------------------------
void A3::uploadVertexDataToVbos (
		const MeshConsolidator & meshConsolidator
) {
	// Generate VBO to store all vertex position data
	{
		glGenBuffers(1, &m_vbo_vertexPositions);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexPositions);

		glBufferData(GL_ARRAY_BUFFER, meshConsolidator.getNumVertexPositionBytes(),
				meshConsolidator.getVertexPositionDataPtr(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	// Generate VBO to store all vertex normal data
	{
		glGenBuffers(1, &m_vbo_vertexNormals);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexNormals);

		glBufferData(GL_ARRAY_BUFFER, meshConsolidator.getNumVertexNormalBytes(),
				meshConsolidator.getVertexNormalDataPtr(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	// Generate VBO to store the trackball circle.
	{
		glGenBuffers( 1, &m_vbo_arcCircle );
		glBindBuffer( GL_ARRAY_BUFFER, m_vbo_arcCircle );

		float *pts = new float[ 2 * CIRCLE_PTS ];
		for( size_t idx = 0; idx < CIRCLE_PTS; ++idx ) {
			float ang = 2.0f * float(M_PI) * float(idx) / CIRCLE_PTS;
			pts[2*idx] = cos( ang );
			pts[2*idx+1] = sin( ang );
		}

		glBufferData(GL_ARRAY_BUFFER, 2*CIRCLE_PTS*sizeof(float), pts, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}
}

//----------------------------------------------------------------------------------------
void A3::mapVboDataToVertexShaderInputLocations()
{
	// Bind VAO in order to record the data mapping.
	glBindVertexArray(m_vao_meshData);

	// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
	// "position" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexPositions);
	glVertexAttribPointer(m_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Tell GL how to map data from the vertex buffer "m_vbo_vertexNormals" into the
	// "normal" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexNormals);
	glVertexAttribPointer(m_normalAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	//-- Unbind target, and restore default values:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;

	// Bind VAO in order to record the data mapping.
	glBindVertexArray(m_vao_arcCircle);

	// Tell GL how to map data from the vertex buffer "m_vbo_arcCircle" into the
	// "position" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_arcCircle);
	glVertexAttribPointer(m_arc_positionAttribLocation, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	//-- Unbind target, and restore default values:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
void A3::initPerspectiveMatrix()
{
	float aspect = ((float)m_windowWidth) / m_windowHeight;
	m_perpsective = glm::perspective(degreesToRadians(60.0f), aspect, 0.1f, 100.0f);
}


//----------------------------------------------------------------------------------------
void A3::initViewMatrix() {
	m_view = glm::lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f),
			vec3(0.0f, 1.0f, 0.0f));
}

//----------------------------------------------------------------------------------------
void A3::initLightSources() {
	// World-space position
	m_light.position = vec3(-2.0f, 5.0f, 0.5f);
	m_light.rgbIntensity = vec3(0.8f); // White light
}

//----------------------------------------------------------------------------------------
void A3::uploadCommonSceneUniforms() {
	m_shader.enable();
	{
		//-- Set Perpsective matrix uniform for the scene:
		GLint location = m_shader.getUniformLocation("Perspective");
		glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(m_perpsective));
		CHECK_GL_ERRORS;


		//-- Set LightSource uniform for the scene:
		{
			location = m_shader.getUniformLocation("light.position");
			glUniform3fv(location, 1, value_ptr(m_light.position));
			location = m_shader.getUniformLocation("light.rgbIntensity");
			glUniform3fv(location, 1, value_ptr(m_light.rgbIntensity));
			CHECK_GL_ERRORS;
		}

		//-- Set background light ambient intensity
		{
			location = m_shader.getUniformLocation("ambientIntensity");
			vec3 ambientIntensity(0.05f);
			glUniform3fv(location, 1, value_ptr(ambientIntensity));
			CHECK_GL_ERRORS;
		}
	}
	m_shader.disable();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A3::appLogic()
{
	// Place per frame, application logic here ...

	uploadCommonSceneUniforms();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void A3::guiLogic()
{
	if( !show_gui ) {
		return;
	}

	static bool firstRun(true);
	if (firstRun) {
		ImGui::SetNextWindowPos(ImVec2(50, 50));
		firstRun = false;
	}

	static bool showDebugWindow(true);
	ImGuiWindowFlags windowFlags(ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_AlwaysAutoResize);
	float opacity(0.5f);

	ImGui::Begin("Properties", &showDebugWindow, ImVec2(550,550), opacity, windowFlags);
	{
		if (ImGui::BeginMenuBar()) {

			if (ImGui::BeginMenu("Application")) {
				if (ImGui::MenuItem("Reset Position (I)")) {
					resetPosition();
				}
				if (ImGui::MenuItem("Reset Orientation (O)")) {
					resetOrientation();
				}
				if (ImGui::MenuItem("Reset Joints (N)")) {
					resetJoints();
				}
				if (ImGui::MenuItem("Reset All (A)")) {
					resetAll();
				}
				if (ImGui::MenuItem("Quit (Q)")) {
					quit();
				}

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit")) {
				if (ImGui::MenuItem("Undo (U)", nullptr, nullptr, canUndo())) {
					undo();
				}
				if (ImGui::MenuItem("Redo (R)", nullptr, nullptr, canRedo())) {
					redo();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Options")) {
				if (ImGui::MenuItem("Circle (C)", nullptr, &m_drawCircle)) {
				}
				if (ImGui::MenuItem("Z-buffer (Z)", nullptr, &m_useZBuffer)) {
				}
				if (ImGui::MenuItem("Backface culling (B)", nullptr, &m_backfaceCulling)) {
				}
				if (ImGui::MenuItem("Frontface culling (F)", nullptr, &m_frontfaceCulling)) {
				}
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		ImGui::PushID(0);
		if (ImGui::RadioButton("Position/Orientation (P)", &m_jointMode, 0)) {
		}
		ImGui::PopID();
		ImGui::PushID(1);
		if (ImGui::RadioButton("Joints (J)", &m_jointMode, 1)) {
		}
		ImGui::PopID();

		ImGui::Text("Framerate: %.1f FPS", ImGui::GetIO().Framerate);

		const mat4& rotMat = m_rootRotate;
		ImGui::Text("Rotation:");
		ImGui::Text("%.2f %.2f %.2f %.2f", rotMat[0][0], rotMat[1][0], rotMat[2][0], rotMat[3][0]);
		ImGui::Text("%.2f %.2f %.2f %.2f", rotMat[0][1], rotMat[1][1], rotMat[2][1], rotMat[3][1]);
		ImGui::Text("%.2f %.2f %.2f %.2f", rotMat[0][2], rotMat[1][2], rotMat[2][2], rotMat[3][2]);
		ImGui::Text("%.2f %.2f %.2f %.2f", rotMat[0][3], rotMat[1][3], rotMat[2][3], rotMat[3][3]);

		const mat4& transMat = m_rootTranslate;
		ImGui::Text("Translation:");
		ImGui::Text("%.2f %.2f %.2f %.2f", transMat[0][0], transMat[1][0], transMat[2][0], transMat[3][0]);
		ImGui::Text("%.2f %.2f %.2f %.2f", transMat[0][1], transMat[1][1], transMat[2][1], transMat[3][1]);
		ImGui::Text("%.2f %.2f %.2f %.2f", transMat[0][2], transMat[1][2], transMat[2][2], transMat[3][2]);
		ImGui::Text("%.2f %.2f %.2f %.2f", transMat[0][3], transMat[1][3], transMat[2][3], transMat[3][3]);


		float pixelRGB[3];
		glReadPixels(
			static_cast<int>(m_mousePos.x),
			m_framebufferHeight - static_cast<int>(m_mousePos.y),
			1, 1,
			GL_RGB,
			GL_FLOAT,
			pixelRGB
		);
		ImGui::Text("Mouse pos: (%.1f, %.1f)", m_mousePos.x, m_mousePos.y);
		ImGui::Text("Colour under cursor: (%.1f, %.1f, %.1f)", pixelRGB[0], pixelRGB[1], pixelRGB[2]);
	}
	ImGui::End();
}

//----------------------------------------------------------------------------------------
// Update mesh specific shader uniforms:
static void updateShaderUniforms(
		const ShaderProgram & shader,
		const GeometryNode & node,
		const glm::mat4 & viewMatrix,
		const glm::mat4 & modelMatrix
) {

	shader.enable();
	{
		//-- Set ModelView matrix:
		GLint location = shader.getUniformLocation("ModelView");
		mat4 modelView = viewMatrix * modelMatrix;
		glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(modelView));
		CHECK_GL_ERRORS;

		//-- Set NormMatrix:
		location = shader.getUniformLocation("NormalMatrix");
		mat3 normalMatrix = glm::transpose(glm::inverse(mat3(modelView)));
		glUniformMatrix3fv(location, 1, GL_FALSE, value_ptr(normalMatrix));
		CHECK_GL_ERRORS;


		//-- Set Material values:
		location = shader.getUniformLocation("material.kd");
		vec3 kd = node.getMaterial().kd;
		glUniform3fv(location, 1, value_ptr(kd));
		CHECK_GL_ERRORS;
		location = shader.getUniformLocation("material.ks");
		vec3 ks = node.getMaterial().ks;
		glUniform3fv(location, 1, value_ptr(ks));
		CHECK_GL_ERRORS;
		location = shader.getUniformLocation("material.shininess");
		glUniform1f(location, node.getMaterial().shininess);
		CHECK_GL_ERRORS;

	}
	shader.disable();

}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A3::draw() {
	updateCulling();

	if (useZBuffer()) {
		glEnable(GL_DEPTH_TEST);
	}
	renderSceneGraph(*m_rootNode);

	if (drawCircle()) {
		glDisable(GL_DEPTH_TEST);
		renderArcCircle();
	}
}

//----------------------------------------------------------------------------------------
void A3::renderSceneGraph(const SceneNode & root) {

	// Bind the VAO once here, and reuse for all GeometryNode rendering below.
	glBindVertexArray(m_vao_meshData);

	// This is emphatically *not* how you should be drawing the scene graph in
	// your final implementation.  This is a non-hierarchical demonstration
	// in which we assume that there is a list of GeometryNodes living directly
	// underneath the root node, and that we can draw them in a loop.  It's
	// just enough to demonstrate how to get geometry and materials out of
	// a GeometryNode and onto the screen.

	// You'll want to turn this into recursive code that walks over the tree.
	// You can do that by putting a method in SceneNode, overridden in its
	// subclasses, that renders the subtree rooted at every node.  Or you
	// could put a set of mutually recursive functions in this class, which
	// walk down the tree from nodes of different types.

	pushMatrix();
	multMatrix(m_rootTranslate);
	multMatrix(m_rootRotate);

	root.draw(m_renderSceneNode);

	popMatrix();

	glBindVertexArray(0);
	CHECK_GL_ERRORS;
}

void A3::renderSceneNode(const SceneNode & node) {
	pushMatrix();
	multMatrix(node.getTransform());
}

void A3::renderSceneNode(const GeometryNode & node) {
	renderSceneNode(static_cast<const SceneNode&>(node));

	updateShaderUniforms(m_shader, node, m_view, m_transformStack.back());

	// Get the BatchInfo corresponding to the GeometryNode's unique MeshId.
	BatchInfo batchInfo = m_batchInfoMap[node.getMeshID()];

	//-- Now render the mesh:
	m_shader.enable();
	glDrawArrays(GL_TRIANGLES, batchInfo.startIndex, batchInfo.numIndices);
	m_shader.disable();
}

void A3::renderSceneNode(const JointNode & node) {
	renderSceneNode(static_cast<const SceneNode&>(node));
}

void A3::renderSceneNodePost() {
	popMatrix();
}

//----------------------------------------------------------------------------------------
// Draw the trackball circle.
void A3::renderArcCircle() {
	glBindVertexArray(m_vao_arcCircle);

	m_shader_arcCircle.enable();
		GLint m_location = m_shader_arcCircle.getUniformLocation( "M" );
		float aspect = float(m_framebufferWidth)/float(m_framebufferHeight);
		glm::mat4 M;
		if( aspect > 1.0 ) {
			M = glm::scale( glm::mat4(), glm::vec3( 0.5/aspect, 0.5, 1.0 ) );
		} else {
			M = glm::scale( glm::mat4(), glm::vec3( 0.5, 0.5*aspect, 1.0 ) );
		}
		glUniformMatrix4fv( m_location, 1, GL_FALSE, value_ptr( M ) );
		glDrawArrays( GL_LINE_LOOP, 0, CIRCLE_PTS );
	m_shader_arcCircle.disable();

	glBindVertexArray(0);
	CHECK_GL_ERRORS;
}

void A3::resetPosition() {
}

void A3::resetOrientation() {
}

void A3::resetJoints() {
	m_commandStack.clear();
	m_commandStackPosition = 0;
}

void A3::resetAll() {
	resetPosition();
	resetOrientation();
	resetJoints();
}

void A3::quit() {
	glfwSetWindowShouldClose(m_window, GL_TRUE);
}

void A3::undo()
{
}

void A3::redo()
{
}

bool A3::canUndo() const {
	return m_commandStackPosition > 0;
}

bool A3::canRedo() const
{
	return m_commandStackPosition < m_commandStack.size();
}

bool A3::drawCircle() const {
	return m_drawCircle;
}

bool A3::useZBuffer() const {
	return m_useZBuffer;
}

void A3::updateCulling() {
	if (!m_backfaceCulling && !m_frontfaceCulling) {
		glDisable(GL_CULL_FACE);
	}
	else {
		glEnable(GL_CULL_FACE);
		if (m_backfaceCulling && m_frontfaceCulling) {
			glCullFace(GL_FRONT_AND_BACK);
		}
		else if (m_backfaceCulling) {
			glCullFace(GL_BACK);
		}
		else if (m_frontfaceCulling) {
			glCullFace(GL_FRONT);
		}
	}
}

bool A3::jointMode() const {
	return m_jointMode != 0;
}

void A3::pushMatrix() {
	m_transformStack.push_back(glm::mat4(m_transformStack.back()));
}

void A3::popMatrix() {
	m_transformStack.pop_back();
}

void A3::multMatrix(const glm::mat4& matrix) {
	m_transformStack.back() = m_transformStack.back() * matrix;
}

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void A3::cleanup()
{

}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool A3::cursorEnterWindowEvent (
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
bool A3::mouseMoveEvent (
		double xPos,
		double yPos
) {
	bool eventHandled(false);

	vec2 mousePos(
		static_cast<float>(xPos),
		static_cast<float>(yPos)
	);
	vec2 windowSize(
		static_cast<float>(m_windowWidth),
		static_cast<float>(m_windowHeight)
	);

	vec2 mouseDelta = mousePos - m_mousePos;
	vec3 translateDelta;

	if (m_mouseButtonPressed[GLFW_MOUSE_BUTTON_LEFT]) {
		float xSensitivity = 1.0f / windowSize.x;
		float ySensitivity = -1.0f / windowSize.y;

		translateDelta.x = mouseDelta.x * xSensitivity;
		translateDelta.y = mouseDelta.y * ySensitivity;

		eventHandled = true;
	}
	if (m_mouseButtonPressed[GLFW_MOUSE_BUTTON_MIDDLE]) {
		float zSensitivity = 1.0f / windowSize.y;

		translateDelta.z = mouseDelta.y * zSensitivity;

		eventHandled = true;
	}

	if (translateDelta != vec3()) {
		m_rootTranslate = glm::translate(m_rootTranslate, translateDelta);
	}

	if (m_mouseButtonPressed[GLFW_MOUSE_BUTTON_RIGHT]) {
		mat4 rotateDelta = Trackball::getTrackballRotate(
			m_mousePos,
			mousePos,
			windowSize
		);
		m_rootRotate = m_rootRotate * rotateDelta;

		if (rotateDelta[0][0] == numeric_limits<float>::infinity() ||
			rotateDelta[0][0] == -1.0f * numeric_limits<float>::infinity()) {
			eventHandled = true;
		}

		eventHandled = true;
	}

	m_mousePos = mousePos;

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool A3::mouseButtonInputEvent (
		int button,
		int actions,
		int mods
) {
	bool eventHandled(false);

	if (button >= 0 && button < NumMouseButtons) {
		if (actions == GLFW_PRESS) {
			m_mouseButtonPressed[button] = true;
		}
		else if (actions == GLFW_RELEASE) {
			m_mouseButtonPressed[button] = false;
		}
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool A3::mouseScrollEvent (
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
bool A3::windowResizeEvent (
		int width,
		int height
) {
	bool eventHandled(false);
	initPerspectiveMatrix();

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool A3::keyInputEvent (
		int key,
		int action,
		int mods
) {
	bool eventHandled(false);

	if( action == GLFW_PRESS ) {
		if( key == GLFW_KEY_M ) {
			show_gui = !show_gui;
			eventHandled = true;
		}
		if (key == GLFW_KEY_I) {
			resetPosition();
			eventHandled = true;
		}
		if (key == GLFW_KEY_O) {
			resetOrientation();
			eventHandled = true;
		}
		if (key == GLFW_KEY_N) {
			resetJoints();
			eventHandled = true;
		}
		if (key == GLFW_KEY_A) {
			resetAll();
			eventHandled = true;
		}
		if (key == GLFW_KEY_Q) {
			quit();
			eventHandled = true;
		}
		if (key == GLFW_KEY_U) {
			if (canUndo()) {
				undo();
			}
			eventHandled = true;
		}
		if (key == GLFW_KEY_R) {
			if (canRedo()) {
				redo();
			}
			eventHandled = true;
		}
		if (key == GLFW_KEY_C) {
			m_drawCircle = !m_drawCircle;
			eventHandled = true;
		}
		if (key == GLFW_KEY_Z) {
			m_useZBuffer = !m_useZBuffer;
			eventHandled = true;
		}
		if (key == GLFW_KEY_B) {
			m_backfaceCulling = !m_backfaceCulling;
			eventHandled = true;
		}
		if (key == GLFW_KEY_F) {
			m_frontfaceCulling = !m_frontfaceCulling;
			eventHandled = true;
		}
		if (key == GLFW_KEY_P) {
			m_jointMode = 0;
			eventHandled = true;
		}
		if (key == GLFW_KEY_J) {
			m_jointMode = 1;
			eventHandled = true;
		}
	}

	return eventHandled;
}

A3::RenderSceneNode::RenderSceneNode(A3 & a3)
	: m_a3(a3) {
}

void A3::RenderSceneNode::renderSceneNode(const SceneNode & node) {
	m_a3.renderSceneNode(node);
}

void A3::RenderSceneNode::renderSceneNode(const GeometryNode & node) {
	m_a3.renderSceneNode(node);
}

void A3::RenderSceneNode::renderSceneNode(const JointNode & node) {
	m_a3.renderSceneNode(node);
}

void A3::RenderSceneNode::renderSceneNodePost() {
	m_a3.renderSceneNodePost();
}
