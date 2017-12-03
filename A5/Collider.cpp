#include "Collider.h"

#include "cs488-framework/OpenGLImport.hpp"

using namespace glm;

Collider::Collider(glm::vec2 position, float radius)
	: m_position(position)
	, m_radius(radius)
{
}

void Collider::debugDraw()
{
#if RENDER_DEBUG
	glColor3f(0, 1.0f, 0);
	glBegin(GL_LINES);

	float x = m_position.x;
	float y = 1000.0f;
	float z = m_position.y;
	float r = m_radius;

	glVertex3f(x - r, -y, z - r);
	glVertex3f(x - r,  y, z - r);

	glVertex3f(x - r, -y, z + r);
	glVertex3f(x - r,  y, z + r);

	glVertex3f(x + r, -y, z - r);
	glVertex3f(x + r,  y, z - r);

	glVertex3f(x + r, -y, z + r);
	glVertex3f(x + r,  y, z + r);

	glEnd();
#endif
}
