#include "JointNode.hpp"

#include "A3.hpp"

#include "cs488-framework/MathUtils.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

//---------------------------------------------------------------------------------------
JointNode::JointNode(const std::string& name)
	: SceneNode(name)
{
}

//---------------------------------------------------------------------------------------
JointNode::~JointNode() {

}

void JointNode::initializeTree()
{
	if (m_jointX.min != m_jointX.max || m_jointY.min != m_jointY.max) {
		for (auto& child : m_children) {
			child->setParentJoint(this);
		}
	}

	SceneNode::initializeTree();
}

void JointNode::resetTree()
{
	setRotation(glm::vec2(m_jointX.init, m_jointY.init));

	SceneNode::resetTree();
}

glm::mat4 JointNode::getTransform() const
{
	glm::mat4 rot =
		glm::rotate(glm::mat4(), m_rotation.x, glm::vec3(1.0f, 0, 0)) *
		glm::rotate(glm::mat4(), m_rotation.y, glm::vec3(0, 1.0f, 0));
	return SceneNode::getTransform() * rot;
}

 //---------------------------------------------------------------------------------------
void JointNode::setJointX(double min, double init, double max) {
	m_jointX.min = static_cast<float>(degreesToRadians(min));
	m_jointX.init = static_cast<float>(degreesToRadians(init));
	m_jointX.max = static_cast<float>(degreesToRadians(max));

	m_rotation.x = m_jointX.init;
}

//---------------------------------------------------------------------------------------
void JointNode::setJointY(double min, double init, double max) {
	m_jointY.min = static_cast<float>(degreesToRadians(min));
	m_jointY.init = static_cast<float>(degreesToRadians(init));
	m_jointY.max = static_cast<float>(degreesToRadians(max));

	m_rotation.y = m_jointY.init;
}

glm::vec2 JointNode::getRotation() const {
	return m_rotation;
}

void JointNode::setRotation(glm::vec2 rotation)
{
	static float highestX = 0;
	highestX = std::max(rotation.x, highestX);

	rotation.x = std::max(std::min(rotation.x, m_jointX.max), m_jointX.min);
	rotation.y = std::max(std::min(rotation.y, m_jointY.max), m_jointY.min);

	m_rotation = rotation;
}

bool JointNode::isHead() const
{
	return m_name.compare("neckJoint") == 0;
}

std::string JointNode::getDebugString() const
{
	return "JointNode";
}

void JointNode::draw(IRenderSceneNode& render) const
{
	render.renderSceneNode(*this);
	SceneNode::drawCommon(render);
}
