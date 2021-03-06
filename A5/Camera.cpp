#include "Camera.hpp"

#include "cs488-framework/MathUtils.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <algorithm>

using namespace glm;

Camera::Camera()
	: m_needToRecalculateViewMatrix(true)
	, m_directionPressed{ false, false, false, false }
	, m_speed(4.0f)
	, m_2dWalkMode(true)
{
}

const mat4& Camera::getViewMatrix()
{
	if (m_needToRecalculateViewMatrix) {
		recalculateViewMatrix();
	}
	return m_viewMatrix;
}

void Camera::update(double deltaTime)
{
	int x = 0;
	int z = 0;
	if (directionPressed(Direction::FORWARD)) {
		z -= 1;
	}
	if (directionPressed(Direction::BACKWARD)) {
		z += 1;
	}
	if (directionPressed(Direction::LEFT)) {
		x -= 1;
	}
	if (directionPressed(Direction::RIGHT)) {
		x += 1;
	}

	if (x != 0 || z != 0) {
		quat lookDir = glm::angleAxis(-m_angle.x, vec3(0, 1.0f, 0));

		if (!m_2dWalkMode) {
			lookDir = glm::rotate(lookDir, -m_angle.y, vec3(1.0f, 0, 0));
		}

		float speed = static_cast<float>(m_speed * deltaTime);
		float fx = static_cast<float>(x);
		float fz = static_cast<float>(z);
		vec3 moveVec = speed * normalize(vec3(fx, 0.0f, fz));
		moveVec = glm::rotate(lookDir, moveVec);

		moveTo(m_position + moveVec);
	}
}

vec3 Camera::getPosition() const {
	return m_position;
}

bool Camera::get2dWalkMode() const {
	return m_2dWalkMode;
}

void Camera::moveTo(glm::vec3 pos)
{
	m_position = pos;
	m_needToRecalculateViewMatrix = true;
}

void Camera::rotate(glm::vec2 angleDelta)
{
	m_angle += angleDelta;

	float maxY = degreesToRadians(90.0f);
	float minY = degreesToRadians(-90.0f);
	m_angle.y = std::min(m_angle.y, maxY);
	m_angle.y = std::max(m_angle.y, minY);

	m_needToRecalculateViewMatrix = true;
}

void Camera::setSpeed(float speed)
{
	m_speed = speed;
}

void Camera::set2dWalkMode(bool use2dWalkMode) {
	m_2dWalkMode = use2dWalkMode;
}

void Camera::setDirectionPressed(Direction dir, bool pressed)
{
	directionPressed(dir) = pressed;
}

void Camera::recalculateViewMatrix()
{
	m_needToRecalculateViewMatrix = false;

	mat4 trans = glm::translate(mat4(), vec3() - m_position);

	quat lookDir = glm::angleAxis(m_angle.y, vec3(1.0f, 0, 0));
	lookDir = glm::rotate(lookDir, m_angle.x, vec3(0, 1.0f, 0));
	mat4 rot = glm::toMat4(lookDir);

	m_viewMatrix = rot * trans;
}

bool& Camera::directionPressed(Direction dir)
{
	return m_directionPressed[static_cast<std::size_t>(dir)];
}
