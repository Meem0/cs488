#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
	Camera();

	const glm::mat4& getViewMatrix();

	void update(double deltaTime);

	glm::vec3 getPosition() const;
	bool get2dWalkMode() const;

	void moveTo(glm::vec3 pos);
	void rotate(glm::vec2 angleDelta);

	void setSpeed(float speed);
	void set2dWalkMode(bool use2dWalkMode);

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

	float m_speed;
	bool m_2dWalkMode;

	bool& directionPressed(Direction);
	bool m_directionPressed[static_cast<std::size_t>(Direction::NUM)];
};
