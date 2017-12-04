#pragma once

#define _USE_MATH_DEFINES

#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"

#include "Window.hpp"

#include <glm/glm.hpp>

#include <functional>
#include <vector>

#include "Camera.hpp"
#include "Collider.h"
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
	void createTerrainGeometry(std::function<float(float, float)> heightFunction);

	float getTerrainHeight(glm::vec2 pos) const;

	void drawTerrain();
	void drawTrees(GLint uniformM);

	glm::mat4 calculateLightSpaceMatrix() const;
	void renderDepthBuffer();

	void setShowMouse(bool showMouse);
	bool m_showMouse;

	bool m_wireframeMode;
	bool m_multisample;
	bool m_useBumpMap;

	glm::vec3 m_lightPosition;
	float m_lightIntensity;

	glm::vec3 m_ambientIntensity;

	glm::vec3 m_specularCoeff;
	float m_shininess;

	float m_movementSpeed;

	float m_terrainWidth;
	float m_terrainTileCountSlider;
	std::uint32_t m_terrainTileCount;
	const static std::uint32_t MaxTiles = 1024;

	std::size_t m_octaves;
	std::vector<float> m_heightScaleFactor;
	std::vector<float> m_frequencyFactor;
	int m_seed;

	// Fields related to the shader and uniforms.
	ShaderProgram m_shader;
	GLint m_uniformP; // Uniform location for Projection matrix.
	GLint m_uniformV; // Uniform location for View matrix.
	GLint m_uniformM; // Uniform location for Model matrix.
	GLint m_uniformColour;
	GLint m_uniformLightPosition;
	GLint m_uniformLightSpaceMatrix;
	GLint m_uniformLightColour;
	GLint m_uniformAmbientIntensity;
	GLint m_uniformUseBumpMap;
	GLint m_uniformUseShadows;

	GLuint m_vaoTerrain; // Vertex Array Object
	GLuint m_vboTerrain; // Vertex Buffer Object
	GLuint m_vboTerrainNormals; // Vertex Buffer Object
	GLuint m_vboTerrainuTangents; // Vertex Buffer Object
	GLuint m_vboTerrainvTangents; // Vertex Buffer Object
	GLuint m_vboTerrainTexCoords; // Vertex Buffer Object
	GLuint m_eboTerrain; // Element Buffer Object
	GLuint m_terrainTexture;
	GLuint m_terrainBumpMap;

	ShaderProgram m_skyboxShader;
	GLint m_uniformSkyboxP;
	GLint m_uniformSkyboxV;

	GLuint m_vaoSkybox;
	GLuint m_skyboxCubemap;
	std::size_t m_skyboxIndexCount;

	const std::size_t ShadowWidth = 1024;
	const std::size_t ShadowHeight = 1024;
	ShaderProgram m_depthShader;
	GLint m_uniformDepthM;
	GLint m_uniformDepthLightSpaceMatrix;

	GLuint m_depthBuffer;
	GLuint m_depthMap;
	bool m_useShadows;

	ShaderProgram m_debugQuadShader;
	GLuint m_vaoDebugQuad;
	bool m_renderDebugQuad;

	std::vector<glm::vec3> m_terrainVertices;
#if RENDER_DEBUG
	std::vector<glm::vec3> m_terrainNormals;
#endif
	bool m_normalDebug;

	glm::mat4 m_projMat;

	Camera m_camera;
	std::vector<Tree> m_trees;
	std::vector<Collider> m_colliders;
};
