#include "SceneNode.hpp"

#include "cs488-framework/MathUtils.hpp"

#include "A3.hpp"

#include <iostream>
#include <sstream>
using namespace std;

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

using namespace glm;

namespace NodeUtils {
	unsigned int colourToId(vec3 colour) {
		return 0;
	}

	vec3 idToColour(unsigned int id) {
		return vec3();
	}
}


// Static class variable
unsigned int SceneNode::s_nodeInstanceCount = 0;


//---------------------------------------------------------------------------------------
SceneNode::SceneNode(const std::string& name)
  : m_name(name),
	m_trans(mat4()),
	m_isSelected(false),
	m_nodeId(s_nodeInstanceCount++)
{

}

//---------------------------------------------------------------------------------------
SceneNode::~SceneNode() {
}

//---------------------------------------------------------------------------------------
void SceneNode::setTransform(const glm::mat4& m) {
	m_trans = m;
	m_invtrans = m;
}

//---------------------------------------------------------------------------------------
const glm::mat4& SceneNode::getTransform() const {
	return m_trans;
}

//---------------------------------------------------------------------------------------
const glm::mat4& SceneNode::getInverse() const {
	return m_invtrans;
}

unsigned int SceneNode::getNodeId() const
{
	return m_nodeId;
}

//---------------------------------------------------------------------------------------
void SceneNode::add_child(SceneNode* child) {
	unique_ptr<SceneNode> ptr(child);
	m_children.push_back(move(ptr));
}

//---------------------------------------------------------------------------------------
void SceneNode::rotate(char axis, float angle) {
	vec3 rot_axis;

	switch (axis) {
		case 'x':
			rot_axis = vec3(1,0,0);
			break;
		case 'y':
			rot_axis = vec3(0,1,0);
	        break;
		case 'z':
			rot_axis = vec3(0,0,1);
	        break;
		default:
			break;
	}
	mat4 rot_matrix = glm::rotate(degreesToRadians(angle), rot_axis);
	m_trans = rot_matrix * m_trans;
}

//---------------------------------------------------------------------------------------
void SceneNode::scale(const glm::vec3 & amount) {
	m_trans = glm::scale(amount) * m_trans;
}

//---------------------------------------------------------------------------------------
void SceneNode::translate(const glm::vec3& amount) {
	m_trans = glm::translate(amount) * m_trans;
}

void SceneNode::draw(IRenderSceneNode& render) const
{
	render.renderSceneNode(*this);
	SceneNode::drawCommon(render);
}

bool SceneNode::toggleSelected(unsigned int id)
{
	if (m_nodeId == id) {
		m_isSelected = !m_isSelected;
		return true;
	}

	for (const auto& child : m_children) {
		if (child->toggleSelected(id)) {
			return true;
		}
	}

	return false;
}

bool SceneNode::isSelected() const
{
	return m_isSelected;
}

std::string SceneNode::getDebugString() const
{
	return "SceneNode";
}

void SceneNode::drawCommon(IRenderSceneNode& render) const
{
	for (const auto& child : m_children) {
		child->draw(render);
	}
	render.renderSceneNodePost();
}


//---------------------------------------------------------------------------------------
int SceneNode::totalSceneNodes() const {
	return s_nodeInstanceCount;
}

//---------------------------------------------------------------------------------------
std::ostream & operator << (std::ostream & os, const SceneNode & node) {

	//os << "SceneNode:[NodeType: ___, name: ____, id: ____, isSelected: ____, transform: ____"
	os << node.getDebugString();
	os << ":[";

	os << "name:" << node.m_name << ", ";
	os << "id:" << node.m_nodeId;
	os << "]";

	return os;
}
