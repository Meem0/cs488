#pragma once

#include <glm/glm.hpp>

class Collider {
public:
	Collider(glm::vec2 position, float radius);
	void debugDraw() const;

	bool collide(glm::vec2 position, float radius) const;

	glm::vec2 getPosition() const;
	float getRadius() const;

private:
	glm::vec2 m_position;
	float m_radius;
};
