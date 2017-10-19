#include "JointNode.hpp"

#include "A3.hpp"

//---------------------------------------------------------------------------------------
JointNode::JointNode(const std::string& name)
	: SceneNode(name)
{
}

//---------------------------------------------------------------------------------------
JointNode::~JointNode() {

}
 //---------------------------------------------------------------------------------------
void JointNode::setJointX(double min, double init, double max) {
	m_jointX.min = min;
	m_jointX.init = init;
	m_jointX.max = max;
}

//---------------------------------------------------------------------------------------
void JointNode::setJointY(double min, double init, double max) {
	m_jointY.min = min;
	m_jointY.init = init;
	m_jointY.max = max;
}

std::string JointNode::getDebugString() const
{
	return "JointNode";
}

void JointNode::draw(IRenderSceneNode& render) const
{
	render.renderSceneNode(*this);
	SceneNode::draw(render);
}
