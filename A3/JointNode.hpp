#pragma once

#include "SceneNode.hpp"

class JointNode : public SceneNode {
public:
	JointNode(const std::string & name);
	virtual ~JointNode();

	virtual void initializeTree() override;

	virtual void draw(IRenderSceneNode& render) const override;

	void setJointX(double min, double init, double max);
	void setJointY(double min, double init, double max);

	glm::vec2 getRotation() const;
	void setRotation(glm::vec2);

	bool isHead() const;

protected:
	virtual std::string getDebugString() const override;

private:
	struct JointRange {
		float min, init, max;
	};

	JointRange m_jointX;
	JointRange m_jointY;

	glm::vec2 m_rotation;
};
