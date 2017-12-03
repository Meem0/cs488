#pragma once

#include <glm/glm.hpp>

class Collider {
public:
	Collider(glm::vec2 position, float radius);
	void debugDraw();

private:
	glm::vec2 m_position;
	float m_radius;
};
