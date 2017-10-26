#pragma once

#include "SceneNode.hpp"

class GeometryNode : public SceneNode {
public:
	GeometryNode(
		const std::string & meshId,
		const std::string & name
	);

	virtual void draw(IRenderSceneNode& render) const override;

	Material& getMaterial();
	const Material& getMaterial() const;
	const std::string& getMeshID() const;

	virtual JointNode* getParentJoint() override;
	virtual void setParentJoint(JointNode*) override;

protected:
	virtual std::string getDebugString() const;

private:
	Material m_material;

	// Mesh Identifier. This must correspond to an object name of
	// a loaded .obj file.
	std::string m_meshId;

	JointNode* m_parentJoint;
};
