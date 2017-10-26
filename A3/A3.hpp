#pragma once

#define _USE_MATH_DEFINES

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"
#include "cs488-framework/MeshConsolidator.hpp"

#include "SceneNode.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <functional>

struct LightSource {
	glm::vec3 position;
	glm::vec3 rgbIntensity;
};

class GeometryNode;
class JointNode;

// ridiculous solution to not wanting node type enum
// plus not wanting to make any new files
class IRenderSceneNode {
public:
	virtual void renderSceneNode(const SceneNode & node) = 0;
	virtual void renderSceneNode(const GeometryNode& node) = 0;
	virtual void renderSceneNode(const JointNode& node) = 0;
	virtual void renderSceneNodePost() = 0;
};

class A3 : public CS488Window {
public:
	A3(const std::string & luaSceneFile);
	virtual ~A3();

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

	//-- One time initialization methods:
	void processLuaSceneFile(const std::string & filename);
	void createShaderProgram();
	void enableVertexShaderInputSlots();
	void uploadVertexDataToVbos(const MeshConsolidator & meshConsolidator);
	void mapVboDataToVertexShaderInputLocations();
	void initViewMatrix();
	void initLightSources();

	void initPerspectiveMatrix();
	void uploadCommonSceneUniforms();
	void renderSceneGraph(SceneNode &node);
	void renderSceneNode(const SceneNode& node);
	void renderSceneNode(const GeometryNode& node);
	void renderSceneNode(const JointNode& node);
	void renderSceneNodePost();
	void renderArcCircle();

	glm::mat4 m_perpsective;
	glm::mat4 m_view;

	LightSource m_light;

	//-- GL resources for mesh geometry data:
	GLuint m_vao_meshData;
	GLuint m_vbo_vertexPositions;
	GLuint m_vbo_vertexNormals;
	GLint m_positionAttribLocation;
	GLint m_normalAttribLocation;
	ShaderProgram m_shader;

	//-- GL resources for trackball circle geometry:
	GLuint m_vbo_arcCircle;
	GLuint m_vao_arcCircle;
	GLint m_arc_positionAttribLocation;
	ShaderProgram m_shader_arcCircle;

	// BatchInfoMap is an associative container that maps a unique MeshId to a BatchInfo
	// object. Each BatchInfo object contains an index offset and the number of indices
	// required to render the mesh with identifier MeshId.
	BatchInfoMap m_batchInfoMap;

	std::string m_luaSceneFile;

	std::unique_ptr<SceneNode> m_rootNode;

private:
	void resetPosition();
	void resetOrientation();
	void resetJoints();
	void resetAll();
	void quit();

	void undo();
	void redo();

	bool canUndo() const;
	bool canRedo() const;

	bool drawCircle() const;
	bool useZBuffer() const;

	void updateCulling();

	bool jointMode() const;

	void pushMatrix();
	void popMatrix();
	void multMatrix(const glm::mat4&);

	void drawPickingMode();
	void colourUnderCursor(GLubyte[4]) const;

	class RenderSceneNode : public IRenderSceneNode {
	public:
		RenderSceneNode(A3& a3);

		virtual void renderSceneNode(const SceneNode & node) override;
		virtual void renderSceneNode(const GeometryNode & node) override;
		virtual void renderSceneNode(const JointNode & node) override;
		virtual void renderSceneNodePost() override;

	private:
		A3& m_a3;
	};
	RenderSceneNode m_renderSceneNode;

	std::vector<glm::mat4> m_transformStack;
	glm::mat4 m_rootRotate;
	glm::mat4 m_rootTranslate;

	std::vector<int> m_commandStack;
	unsigned int m_commandStackPosition;

	bool m_drawCircle;
	bool m_useZBuffer;
	bool m_backfaceCulling;
	bool m_frontfaceCulling;

	int m_jointMode;

	const static int NumMouseButtons = 3;
	bool m_mouseButtonPressed[NumMouseButtons];
	glm::vec2 m_mousePos;

	bool m_pickingMode;
	unsigned int m_pickedId;
	GLubyte m_pickedColour[3];

	unsigned int m_pickingFrames;
};
