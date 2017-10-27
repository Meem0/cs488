#pragma once

#include "Material.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <memory>
#include <string>
#include <iostream>

namespace NodeUtils {
	unsigned int colourToId(glm::vec3 colour);
	glm::vec3 idToColour(unsigned int id);
}

class IRenderSceneNode;
class JointNode;

class SceneNode {
public:
    SceneNode(const std::string & name);

    virtual ~SceneNode();

	virtual void initializeTree();
	virtual void resetTree();
    
	int totalSceneNodes() const;
    
    virtual glm::mat4 getTransform() const;
    const glm::mat4& getInverse() const;
	unsigned int getNodeId() const;
    
    void setTransform(const glm::mat4& m);
    
    void add_child(SceneNode* child);
    
	//-- Transformations:
    void rotate(char axis, float angle);
    void scale(const glm::vec3& amount);
    void translate(const glm::vec3& amount);

	virtual void draw(IRenderSceneNode& render) const;

	SceneNode* getNode(unsigned int id);

	virtual JointNode* getParentJoint();
	virtual void setParentJoint(JointNode*);

	void setSelected(bool);
	bool isSelected() const;

	friend std::ostream & operator << (std::ostream & os, const SceneNode & node);

protected:
	virtual std::string getDebugString() const;

	void drawCommon(IRenderSceneNode& render) const;

	std::vector<std::unique_ptr<SceneNode>> m_children;
	std::string m_name;

private:
	SceneNode(const SceneNode & other) = delete;

	// The number of SceneNode instances.
	static unsigned int s_nodeInstanceCount;

	bool m_isSelected;

	// Transformations
	glm::mat4 m_trans;
	glm::mat4 m_invtrans;

	unsigned int m_nodeId;
};
