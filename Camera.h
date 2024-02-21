#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

//Defines a few options for camera movement
//Abstraction to avoid window-system specific input methods
enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

//Default Camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 60.0f;

//Camera class to process input and calculate angles, vectors, and matrices
class Camera {
public:
	//Camera variables
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 worldUp;

	//euler angles
	float Yaw;
	float Pitch;

	//Camera Options
	float movementSpeed;
	float mouseSensitivity;
	float zoom;


	//Constructor
	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
		float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
		movementSpeed(SPEED), mouseSensitivity(SENSITIVITY), zoom(ZOOM) {
		Position = position;
		worldUp = up;
		Yaw = yaw;
		Pitch = pitch;
		UpdateCameraVectors();
	}

	//constructor with scalar values
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) :
		Front(glm::vec3(0.0f, 0.0f, -1.0f)), movementSpeed(SPEED), mouseSensitivity(SENSITIVITY), zoom(ZOOM) {
		Position = glm::vec3(posX, posY, posZ);
		worldUp = glm::vec3(upX, upY, upZ);
		Yaw = yaw;
		Pitch = pitch;
		UpdateCameraVectors();
	}

	//returns view matrix calculated from euler angles and lookat matrix
	glm::mat4 GetViewMatrix() const {
		return glm::lookAt(Position, Position + Front, Up);
	}

	//Process keyboard input for accepted ENUM above
	void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
		float velocity = movementSpeed * deltaTime;
		if (direction == FORWARD) {
			Position += Front * velocity;
		}
		if (direction == BACKWARD) {
			Position -= Front * velocity;
		}
		if (direction == LEFT) {
			Position -= Right * velocity;
		}
		if (direction == RIGHT) {
			Position += Right * velocity;
		}
		if (direction == UP) {
			Position += Up * velocity;
		}
		if (direction == DOWN) {
			Position -= Up * velocity;
		}
	}

	void ProcessMouseMovement(float xOffset, float yOffset, GLboolean constrainPitch = true) {
		xOffset *= mouseSensitivity;
		yOffset *= mouseSensitivity;

		Yaw += xOffset;
		Pitch += yOffset;

		//If pitch is out of bounds, make sure that screen doesn't get flipped
		if (constrainPitch) {
			if (Pitch > 89.0f) {
				Pitch = 89.0f;
			}
			if (Pitch < -89.0f) {
				Pitch = -89.0f;
			}
		}

		//update vectors using updated euler angles
		UpdateCameraVectors();
	}

	//process input from mouse wheel
	void ProcessMouseScroll(float yOffset) {
		//Flip because of values for movement speed
		movementSpeed += (float)yOffset;
		if (movementSpeed < 1.0f) {
			movementSpeed = 1.0f;
		}
		if (movementSpeed > 10.0f) {
			movementSpeed = 10.0f;
		}
	}

private:
	//calculates vectors from euler angles
	void UpdateCameraVectors() {
		glm::vec3 tempFront;
		tempFront.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		tempFront.y = sin(glm::radians(Pitch));
		tempFront.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		Front = glm::normalize(tempFront);
		//recalculate the right and up vectors
		//normalize to maintain movement speed
		Right = glm::normalize(glm::cross(Front, worldUp));
		Up = glm::normalize(glm::cross(Right, Front));
	}
};
#endif