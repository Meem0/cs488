#pragma once

#include "SceneNode.hpp"

class JointNode : public SceneNode {
public:
	JointNode(const std::string & name);
	virtual ~JointNode();

	virtual void draw(IRenderSceneNode& render) const override;

	void setJointX(double min, double init, double max);
	void setJointY(double min, double init, double max);

protected:
	virtual std::string getDebugString() const override;

private:
	struct JointRange {
		double min, init, max;
	};

	JointRange m_jointX;
	JointRange m_jointY;
};
