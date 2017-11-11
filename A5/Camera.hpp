#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
	Camera();

	const glm::mat4& getViewMatrix();

	void update();

	void moveTo(glm::vec3 pos);
	void rotate(glm::vec2 angleDelta);

	enum class Direction {
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT,
		NUM
	};

	void setDirectionPressed(Direction, bool pressed);

private:
	void recalculateViewMatrix();

	glm::mat4 m_viewMatrix;
	bool m_needToRecalculateViewMatrix;

	glm::vec3 m_position;
	glm::vec2 m_angle;

	bool& directionPressed(Direction);
	bool m_directionPressed[static_cast<std::size_t>(Direction::NUM)];
};
