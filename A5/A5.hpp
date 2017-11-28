#pragma once

#define _USE_MATH_DEFINES

#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"

#include "Window.hpp"

#include <glm/glm.hpp>

#include <vector>

#include "Camera.hpp"
#include "Tree.hpp"

class A5 : public Window {
public:
	A5();
	virtual ~A5();

protected:
	virtual void init() override;
	virtual void appLogic() override;
	virtual void guiLogic() override;
	virtual void draw() override;
	virtual void cleanup() override;

	//-- Virtual callback methods
	virtual bool cursorEnterWindowEvent(int entered) override;
	virtual bool mouseMoveEvent(double xPos, double yPos) override;
	virtual bool mouseButtonInputEvent(int button, int actions, int mods) override;
	virtual bool mouseScrollEvent(double xOffSet, double yOffSet) override;
	virtual bool windowResizeEvent(int width, int height) override;
	virtual bool keyInputEvent(int key, int action, int mods) override;

private:
	const static int NumMouseButtons = 3;
	bool m_mouseButtonPressed[NumMouseButtons];
	glm::vec2 m_mousePos;

	void initGeom();
	void allocateTerrain();
	void createTerrain();

	void setShowMouse(bool showMouse);
	bool m_showMouse;

	bool m_wireframeMode;

	glm::vec3 m_lightPosition;
	float m_lightIntensity;

	glm::vec3 m_ambientIntensity;

	glm::vec3 m_specularCoeff;
	float m_shininess;

	float m_movementSpeed;

	float m_terrainWidth;
	float m_terrainTileCountSlider;
	std::size_t m_terrainTileCount;
	const static std::size_t MaxTiles = 1024;

	float m_heightScaleFactor;

	// Fields related to the shader and uniforms.
	ShaderProgram m_shader;
	GLint m_uniformP; // Uniform location for Projection matrix.
	GLint m_uniformV; // Uniform location for View matrix.
	GLint m_uniformM; // Uniform location for Model matrix.
	GLint m_uniformColour;
	GLint m_uniformLightPosition;
	GLint m_uniformLightColour;
	GLint m_uniformAmbientIntensity;
	GLint m_uniformSpecularCoeff;
	GLint m_uniformShininess;

	GLuint m_vaoTerrain; // Vertex Array Object
	GLuint m_vboTerrain; // Vertex Buffer Object
	GLuint m_vboTerrainNormals; // Vertex Buffer Object
	GLuint m_vboTerrainTexCoords; // Vertex Buffer Object
	GLuint m_eboTerrain; // Element Buffer Object
	GLuint m_terrainTexture;

	glm::mat4 m_projMat;

	Camera m_camera;
	std::vector<Tree> m_trees;
};
