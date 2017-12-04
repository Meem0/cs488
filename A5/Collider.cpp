#include "Collider.h"

#include "cs488-framework/OpenGLImport.hpp"

#include "Utility.hpp"

using namespace glm;

Collider::Collider(glm::vec2 position, float radius)
	: m_position(position)
	, m_radius(radius)
{
}

void Collider::debugDraw() const
{
#if RENDER_DEBUG
	glColor3f(0, 1.0f, 0);
	glBegin(GL_LINES);

	float x = m_position.x;
	float y = 1000.0f;
	float z = m_position.y;
	float r = m_radius;
	float a = glm::sqrt((r * r) / 2);

	glVertex3f(x - r, -y, z);
	glVertex3f(x - r,  y, z);

	glVertex3f(x + r, -y, z);
	glVertex3f(x + r,  y, z);

	glVertex3f(x, -y, z + r);
	glVertex3f(x,  y, z + r);

	glVertex3f(x, -y, z + r);
	glVertex3f(x,  y, z + r);

	glVertex3f(x - a, -y, z - a);
	glVertex3f(x - a,  y, z - a);

	glVertex3f(x - a, -y, z + a);
	glVertex3f(x - a,  y, z + a);

	glVertex3f(x + a, -y, z - a);
	glVertex3f(x + a,  y, z - a);

	glVertex3f(x + a, -y, z + a);
	glVertex3f(x + a,  y, z + a);

	glEnd();
#endif
}

bool Collider::collide(glm::vec2 position, float radius) const
{
	float minDistance = radius + m_radius;
	float distance = Util::distanceSquared(position, m_position);

	return distance < (minDistance * minDistance);
}
