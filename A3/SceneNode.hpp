#pragma once

#include "Material.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <memory>
#include <string>
#include <iostream>

class IRenderSceneNode;

class SceneNode {
public:
    SceneNode(const std::string & name);

    virtual ~SceneNode();
    
	int totalSceneNodes() const;
    
    const glm::mat4& getTransform() const;
    const glm::mat4& getInverse() const;
    
    void setTransform(const glm::mat4& m);
    
    void add_child(SceneNode* child);
    
	//-- Transformations:
    void rotate(char axis, float angle);
    void scale(const glm::vec3& amount);
    void translate(const glm::vec3& amount);

	virtual void draw(IRenderSceneNode& render) const;

	friend std::ostream & operator << (std::ostream & os, const SceneNode & node);

protected:
	virtual std::string getDebugString() const;

private:
	SceneNode(const SceneNode & other) = delete;

	// The number of SceneNode instances.
	static unsigned int s_nodeInstanceCount;

	bool m_isSelected;

	// Transformations
	glm::mat4 m_trans;
	glm::mat4 m_invtrans;

	std::vector<std::unique_ptr<SceneNode>> m_children;

	std::string m_name;
	unsigned int m_nodeId;
};
