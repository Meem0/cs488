#include "GeometryNode.hpp"

#include "A3.hpp"

//---------------------------------------------------------------------------------------
GeometryNode::GeometryNode(
	const std::string & meshId,
	const std::string & name
)
	: SceneNode(name)
	, m_meshId(meshId)
	, m_parentJoint(nullptr)
{
}

void GeometryNode::draw(IRenderSceneNode& render) const
{
	render.renderSceneNode(*this);
	SceneNode::drawCommon(render);
}

Material & GeometryNode::getMaterial()
{
	return const_cast<Material&>((static_cast<const GeometryNode&>(*this).getMaterial()));
}

const Material & GeometryNode::getMaterial() const
{
	return m_material;
}

const std::string & GeometryNode::getMeshID() const
{
	return m_meshId;
}

JointNode* GeometryNode::getParentJoint()
{
	return m_parentJoint;
}

void GeometryNode::setParentJoint(JointNode* joint)
{
	m_parentJoint = joint;
}

std::string GeometryNode::getDebugString() const
{
	return "GeometryNode";
}
