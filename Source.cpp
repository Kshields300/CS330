#include <iostream>		//for error messages
#include <cstdlib>		//error exception throw
#include <GL/glew.h>	//one of the GL render libraries
#include <GLFW/glfw3.h>	//The other GL render library we need

//Texture loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//glm math headers for 3d calculations
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <math.h>

#include "Camera.h"

//GLSL shader macro
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

//Pi for making the circles
const float PI = 3.1415927f;

//Global variables
//---------------------------------------------------------------------------------
//constants for window and res
const char* WINDOW_TITLE = "CS330 Project";
const int SCREEN_H = 600;
const int SCREEN_W = 800;

//GL mesh struct for vbo and vaos
struct GLMesh {
	GLuint vao;			//vertex array
	GLuint vbos[2];		//vertex buffer for vertices and indices
	GLuint nIndices;
};

//GL initialization
GLFWwindow* window = nullptr;
//Triangle mesh data
GLMesh gMesh;
GLMesh meshPlane;
GLMesh meshCube;
GLMesh meshPyr;
GLMesh meshCyl;

//Textures
GLuint textureIdSphere;
GLuint textureIdPlane;
GLuint textureIdCube;
GLuint textureIdPyr;
GLuint textureIdCyl;
glm::vec2 textureScale(1.0f, 1.0f);
GLint textureWrapMode = GL_REPEAT;

//Shader program init
GLuint shaderProgramId;
GLuint lampProgramId;
GLuint fillProgramId;

//Camera
Camera camera(glm::vec3(-1.0f, 0.0f, 4.0f));
//Initialize to center space
float lastX = SCREEN_W / 2.0f;
float lastY = SCREEN_H / 2.0f;
bool firstMouse = true;

//globalize perspective due to input function
glm::mat4 projection;

//timing
//time between current frame, last frame
float deltaTime = 0.0f;
float lastFrame = 0.0f;

//Plane functions
glm::vec3 planePos(0.0f, 1.8f, 0.0f);
glm::vec3 planeColor(0.5f, 0.2f, 0.0f);
glm::vec3 planeScale(1.0f, 1.0f, 1.0f);

//Game piece functions
glm::vec3 piecePos(-1.1f, 0.0f, 0.0f);
glm::vec3 pieceColor(1.0f, 0.6f, 0.0f);
glm::vec3 pieceScale(0.4f);

//Cube functions
glm::vec3 cubePos(0.0f, -0.75f, 0.2f);
glm::vec3 cubeColor(0.9f, 1.0f, 1.0f);
glm::vec3 cubeScale(0.45f, 0.45f, 0.45f);

//Pyramid functions
glm::vec3 pyrPos(0.8f, -0.8f, -1.0f);
glm::vec3 pyrColor(0.0f, 0.8f, 0.2f);
glm::vec3 pyrScale(0.8f, 0.8f, 0.8f);

//Cylinder functions
glm::vec3 cylPos(-1.0f, -1.2f, -2.0f);
glm::vec3 cylColor(0.5f, 0.5f, 0.5f);
glm::vec3 cylScale(1.0f, 1.0f, 1.0f);

//lamp functions
glm::vec3 lampPos(-8.0f, 0.0f, 0.0f);
glm::vec3 lampColor(1.0f, 1.0f, 1.0f);
glm::vec3 lampScale(0.5f);

//fill light function
glm::vec3 fillPos(0.0f, 5.0f, 0.0f);
glm::vec3 fillColor(1.0f, 1.0f, 0.0f);
glm::vec3 fillScale(0.5f);

//--------------------------------------------------------------------------------------
//Function calls for main
void flipImageVertically(unsigned char* image, int width, int height, int channels);
bool Initialize(int argc, char* argv[], GLFWwindow** window);
void ResizeWindow(GLFWwindow* window, int width, int height);
void ProcessInput(GLFWwindow* window);
//Functions for mouse tracking for camera
void MousePositionCallback(GLFWwindow* window, double xPos, double yPos);
void MouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset);
//Default mesh, contains game piece right now
void CreateMesh(GLMesh& mesh);
//Mesh for the plane, since vertices and indices are hardcoded, need a function for each one
//as well as a separate mesh object for each one
void CreateMeshPlane(GLMesh& mesh);
void CreateMeshCube(GLMesh& mesh);
void CreateMeshPyramid(GLMesh& mesh);
void CreateMeshCylinder(GLMesh& mesh);
void DestroyMesh(GLMesh& mesh);
//Texture functions
bool CreateTexture(const char* filename, GLuint& textureId);
void DestroyTexture(GLuint textureId);
void Render();
bool CreateShaderProgram(const char* vertShaderSource,
	const char* fragShaderSource, GLuint& programId);
void DestroyShaderProgram(GLuint programId);

//------------------------------------------------------------------------------------------
//GLSL calls for shaders
const GLchar* vertexShaderSource = GLSL(440,
	layout(location = 0) in vec3 position;				//vertex data for the shape itself
	layout(location = 1) in vec3 normal;				//lighting data
	layout(location = 2) in vec2 textureCoordinate;		//texture data

	out vec3 vertexNormal;								//Normals for lighting
	out vec3 vertexFragmentPos;							//Outgoing color/pixels to fragment shader
	out vec2 vertexTextureCoordinate;					//Texture coords

	//Globals for transforming matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main() {
		//establish clip field
		gl_Position = projection * view * model * vec4(position, 1.0f);
		//Get fragment pixel info in world space
		vertexFragmentPos = vec3(model * vec4(position, 1.0f));
		//Input normals fed to output for normals
		vertexNormal = mat3(transpose(inverse(model))) * normal;
		//input texture fed to output data
		vertexTextureCoordinate = textureCoordinate;
	}
);

const GLchar* fragmentShaderSource = GLSL(440,

	in vec3 vertexNormal;					//incoming normal data for lighting reflection
	in vec3 vertexFragmentPos;				//position data for the object
	in vec2 vertexTextureCoordinate;		//Texture data from vert shader

	out vec4 fragmentColor;					//output color info

	uniform vec3 objectColor;
	uniform vec3 lightColor;
	uniform vec3 lightPos;
	uniform vec3 fillColor;
	uniform vec3 fillLightPos;
	uniform vec3 viewPos;
	uniform sampler2D uTexture;
	uniform vec2 textureScale;

	void main() {
		//Phone lighting to generate light components

		//Calculate Ambient Lighting
		float ambientStrength = 0.5f;	//base ambient light strength
		float fillStrength = 0.8f;	//base fill light strength
		//generate the actual ambient color
		vec3 ambient = ambientStrength * lightColor;
		vec3 fill = fillStrength * fillColor;

		//Diffuse lighting
		//normalize to unit vectors
		vec3 norm = normalize(vertexNormal);
		//Calculate distance between light source and fragments on object
		vec3 lightDirection = normalize(lightPos - vertexFragmentPos);
		vec3 fillLightDirection = normalize(fillLightPos - vertexFragmentPos);
		//Calculate diffise impact with dot product of normal and light
		float impact = max(dot(norm, lightDirection), 0.0f);
		float fillImpact = max(dot(norm, fillLightDirection), 0.0f);
		//Generates diffuse light color
		vec3 diffuse = impact * lightColor;
		vec3 fillDiffuse = fillImpact * fillColor;

		//Specular lighting
		float specularIntensity = 0.8f;
		//set specular highlight size
		float highlightSize = 16.0f;
		//Calculate view direction
		vec3 viewDir = normalize(viewPos - vertexFragmentPos);
		//Calculate reflection vector
		vec3 reflectDir = reflect(-lightDirection, norm);
		//Calculate specular component
		float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0f), highlightSize);
		vec3 specular = specularIntensity * specularComponent * lightColor;
		vec3 fillSpecular = specularIntensity * specularComponent * fillColor;

		//texture holds color for all 3 components
		vec4 textureColor = texture(uTexture, vertexTextureCoordinate * textureScale);

		//Calculate phong value
		vec3 phong = (ambient + fill + diffuse + fillDiffuse + specular) * textureColor.xyz;

		//Send lighting results to GPU
		fragmentColor = vec4(phong, 1.0);
	}
);

//GLSL light vertex shader source
const GLchar* lampVertexShaderSource = GLSL(440,
	//vertex position data
	layout(location = 0) in vec3 position;

	//Uniforms
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main() {
		gl_Position = projection * view * model * vec4(position, 1.0f);
	}
);

//GLSL light fragment shader source
const GLchar* lampFragmentShaderSource = GLSL(440,

	out vec4 fragmentColor;

	void main() {
		//set color to white with alpha 1.0
		fragmentColor = vec4(1.0f);
		}
);

//Assignment 6 has fill sources here, include if doesnt work
const GLchar* fillVertexShaderSource = GLSL(440,
	//Vertex position data
	layout(location = 0) in vec3 position;

	//Uniforms
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main() {
		gl_Position = projection * view * model * vec4(position, 1.0f);
	}
);

const GLchar* fillFragmentShaderSource = GLSL(440,
	//outgoing light color to GPU
	out vec4 fragmentColor;

	void main() {
		fragmentColor = vec4(1.0f);
	}
);


//-------------------------------------------------------------------------------------------

int main(int argc, char* argv[]) {
	//window creation error
	if (!Initialize(argc, argv, &window)) {
		return EXIT_FAILURE;
	}

	//Create the info inside our mesh
	CreateMesh(gMesh);
	CreateMeshPlane(meshPlane);
	CreateMeshCube(meshCube);
	CreateMeshPyramid(meshPyr);
	CreateMeshCylinder(meshCyl);

	//Create shader program
	if (!CreateShaderProgram(vertexShaderSource, fragmentShaderSource,
		shaderProgramId)) {
		return EXIT_FAILURE;
	}
	if (!CreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource,
		lampProgramId)) {
		return EXIT_FAILURE;
	}
	if (!CreateShaderProgram(fillVertexShaderSource, fillFragmentShaderSource,
		fillProgramId)) {
		return EXIT_FAILURE;
	}

	glUseProgram(shaderProgramId);

	projection = glm::perspective(glm::radians(camera.zoom),
		(GLfloat)SCREEN_W / (GLfloat)SCREEN_H, 0.1f, 100.0f);

	
	//Background Color in rgb and opacity
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	//render loop
	while (!glfwWindowShouldClose(window)) {

		//new per-frame timing for the camera
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		//input function
		ProcessInput(window);

		//Render the Frame
		Render();

		glfwPollEvents();
	}

	//delete mesh and shader program
	DestroyMesh(gMesh);
	DestroyMesh(meshPlane);
	DestroyMesh(meshCube);
	DestroyMesh(meshPyr);
	DestroyMesh(meshCyl);

	DestroyTexture(textureIdSphere);
	DestroyTexture(textureIdPlane);
	DestroyTexture(textureIdCube);
	DestroyTexture(textureIdPyr);
	DestroyTexture(textureIdCyl);

	DestroyShaderProgram(shaderProgramId);
	DestroyShaderProgram(lampProgramId);
	DestroyShaderProgram(fillProgramId);

	//return function, cleaner than return 0
	exit(EXIT_SUCCESS);
}

//Texture function to correct image flip at load
void flipImageVertically(unsigned char* image, int width, int height, int channels) {
	for (int i = 0; i < height / 2; i++) {
		int index1 = i * width * channels;
		int index2 = (height - 1 - i) * width * channels;

		for (int j = width * channels; j > 0; j--) {
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			index1++;
			index2++;
		}
	}
}

//Initialization function
bool Initialize(int argc, char* argv[], GLFWwindow** window) {
	//GLFW initialization and configuration options
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//GLFW window creation
	*window = glfwCreateWindow(SCREEN_W, SCREEN_H, WINDOW_TITLE, NULL, NULL);
	if (*window == NULL) {
		std::cout << "Error: GLFW window creation" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, ResizeWindow);
	//Mouse tracking
	glfwSetCursorPosCallback(*window, MousePositionCallback);
	glfwSetScrollCallback(*window, MouseScrollCallback);

	//Tell GLFW to capture the mouse
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//GLEW Initialize
	glewExperimental = GL_TRUE;
	GLenum GlewInitResult = glewInit();
	if (GLEW_OK != GlewInitResult) {
		std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
		return false;
	}

	//Display version info
	std::cout << "INFO: OpenGL version: " << glGetString(GL_VERSION) << std::endl;

	return true;
}

//process user key inputs
void ProcessInput(GLFWwindow* window) {
	static const float cameraSpeed = 2.5f;
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	//Keyboard camera movement
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		camera.ProcessKeyboard(FORWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		camera.ProcessKeyboard(LEFT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		camera.ProcessKeyboard(RIGHT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		camera.ProcessKeyboard(DOWN, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		camera.ProcessKeyboard(UP, deltaTime);
	}

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
		//Perspective projection for camera
		projection = glm::perspective(glm::radians(camera.zoom),
			(GLfloat)SCREEN_W / (GLfloat)SCREEN_H, 0.1f, 100.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
		projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
	}
}

//resize view along with window
void ResizeWindow(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

//called when mouse moves
void MousePositionCallback(GLFWwindow* window, double xPos, double yPos) {
	if (firstMouse) {
		lastX = xPos;
		lastY = yPos;
		firstMouse = false;
	}

	float xOffset = xPos - lastX;
	float yOffset = lastY - yPos; //Reverse because y coords go top to bottom

	lastX = xPos;
	lastY = yPos;

	//Call to object function to update
	camera.ProcessMouseMovement(xOffset, yOffset);
}

//whenever mouse scrolls, call this
void MouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
	camera.ProcessMouseScroll(yOffset);
}

//function for rendering each frame
void Render() {
	//Z buffer to handle draw errors in 3d
	glEnable(GL_DEPTH_TEST);

	//Clear background to default
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//set up model matrix object for the game piece
	// For additional objects, copy everything except camera pieces for a separate mesh
	//---------------------------------------------------------------------------------
	//Set place in scene
	glm::mat4 model = glm::translate(piecePos) * glm::scale(pieceScale);

	//camera transformation
	glm::mat4 view = camera.GetViewMatrix();

	//Load texture
	const char* texFilename = "Orange-gloss-plastic.jpg";
	if (!CreateTexture(texFilename, textureIdSphere)) {
		std::cout << "Failed to load texture" << texFilename << std::endl;
		return;
	}

	//VAO and VBO activation
	glBindVertexArray(gMesh.vao);
	//Commenting this line out will remove the object
	glUseProgram(shaderProgramId);
	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(shaderProgramId, "uTexture"), 0);

	//Retrieve and pass matrices to shader program
	GLint modelLoc = glGetUniformLocation(shaderProgramId, "model");
	GLint viewLoc = glGetUniformLocation(shaderProgramId, "view");
	GLint projLoc = glGetUniformLocation(shaderProgramId, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//Reference matrix uniforms from the object shader for color and position
	GLint objectColorLoc = glGetUniformLocation(shaderProgramId, "objectColor");
	//Location for light
	GLint lightColorLoc = glGetUniformLocation(shaderProgramId, "lightColor");
	GLint lightPositionLoc = glGetUniformLocation(shaderProgramId, "lightPos");
	//Reference for fill light
	GLint fillColorLoc = glGetUniformLocation(shaderProgramId, "fillLightColor");
	GLint fillPosLoc = glGetUniformLocation(shaderProgramId, "fillLightPos");
	//Reference for Camera
	GLint viewPositionLoc = glGetUniformLocation(shaderProgramId, "viewPos");

	//pass info to the shader variables
	glUniform3f(objectColorLoc, pieceColor.r, pieceColor.g, pieceColor.b);
	//for main light
	glUniform3f(lightColorLoc, lampColor.r, lampColor.g, lampColor.b);
	glUniform3f(lightPositionLoc, lampPos.x, lampPos.y, lampPos.z);
	//for Fill light
	glUniform3f(fillColorLoc, fillColor.r, fillColor.g, fillColor.b);
	glUniform3f(fillPosLoc, fillPos.x, fillPos.y, fillPos.z);

	//Camera
	const glm::vec3 cameraPosition = camera.Position;
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

	GLint textureScaleLoc = glGetUniformLocation(shaderProgramId, "textureScale");
	glUniform2fv(textureScaleLoc, 1, glm::value_ptr(textureScale));

	//Texture activation
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureIdSphere);

	//Draw triangles, using nIndices means you can use this statement for 3d as well
	glDrawElements(GL_TRIANGLES, gMesh.nIndices, GL_UNSIGNED_SHORT, NULL);
	//--------------------------------------------------------------------------------------

	//set up model matrix object for the plane
	//--------------------------------------------------------------------------------------
	model = glm::translate(planePos) * glm::scale(planeScale);

	//Load texture
	texFilename = "Table-wood.jpg";
	if (!CreateTexture(texFilename, textureIdPlane)) {
		std::cout << "Failed to load texture" << texFilename << std::endl;
		return;
	}


	//texture with the program
	glUniform1i(glGetUniformLocation(shaderProgramId, "uTexture"), 0);

	//Retrieve and pass matrices to shader program
	modelLoc = glGetUniformLocation(shaderProgramId, "model");
	viewLoc = glGetUniformLocation(shaderProgramId, "view");
	projLoc = glGetUniformLocation(shaderProgramId, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//Reference matrix uniforms from object shader for color and position
	objectColorLoc = glGetUniformLocation(shaderProgramId, "objectColor");
	//Location for light
	lightColorLoc = glGetUniformLocation(shaderProgramId, "lightColor");
	lightPositionLoc = glGetUniformLocation(shaderProgramId, "lightPos");
	//Reference for fill light
	fillColorLoc = glGetUniformLocation(shaderProgramId, "fillLightColor");
	fillPosLoc = glGetUniformLocation(shaderProgramId, "fillLightPos");
	//pass info to shader variables
	glUniform3f(objectColorLoc, planeColor.r, planeColor.g, planeColor.b);
	glUniform3f(lightColorLoc, lampColor.r, lampColor.g, lampColor.b);
	glUniform3f(lightPositionLoc, lampPos.x, lampPos.y, lampPos.z);
	//for Fill light
	glUniform3f(fillColorLoc, fillColor.r, fillColor.g, fillColor.b);
	glUniform3f(fillPosLoc, fillPos.x, fillPos.y, fillPos.z);

	glBindVertexArray(meshPlane.vao);

	//Texture activation
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureIdPlane);

	glDrawElements(GL_TRIANGLES, meshPlane.nIndices, GL_UNSIGNED_SHORT, NULL);
	//--------------------------------------------------------------------------------------

	//set up model matrix object for the cube
	//--------------------------------------------------------------------------------------
	model = glm::translate(cubePos) * glm::scale(cubeScale);

	//cube texture
	texFilename = "Dice-faces.jpg";
	if (!CreateTexture(texFilename, textureIdCube)) {
		std::cout << "Failed to load texture" << texFilename << std::endl;
		return;
	}

	glUniform1i(glGetUniformLocation(shaderProgramId, "uTexture"), 0);

	//Retrieve and pass matrices to shader program
	modelLoc = glGetUniformLocation(shaderProgramId, "model");
	viewLoc = glGetUniformLocation(shaderProgramId, "view");
	projLoc = glGetUniformLocation(shaderProgramId, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//Reference matrix uniforms from the object shader for color and position
	objectColorLoc = glGetUniformLocation(shaderProgramId, "objectColor");
	//location for light
	lightColorLoc = glGetUniformLocation(shaderProgramId, "lightColor");
	lightPositionLoc = glGetUniformLocation(shaderProgramId, "lightPos");
	//Reference for fill light
	fillColorLoc = glGetUniformLocation(shaderProgramId, "fillLightColor");
	fillPosLoc = glGetUniformLocation(shaderProgramId, "fillLightPos");

	//pass info to shader variables
	glUniform3f(objectColorLoc, cubeColor.r, cubeColor.g, cubeColor.b);
	//for main light
	glUniform3f(lightColorLoc, lampColor.r, lampColor.g, lampColor.b);
	glUniform3f(lightPositionLoc, lampPos.x, lampPos.y, lampPos.z);
	//for Fill light
	glUniform3f(fillColorLoc, fillColor.r, fillColor.g, fillColor.b);
	glUniform3f(fillPosLoc, fillPos.x, fillPos.y, fillPos.z);

	glBindVertexArray(meshCube.vao);

	//Texture activation
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureIdCube);
	
	glDrawElements(GL_TRIANGLES, meshCube.nIndices, GL_UNSIGNED_SHORT, NULL);
	//--------------------------------------------------------------------------------------

	//set up model matrix object for the pyramid
	//--------------------------------------------------------------------------------------
	model = glm::translate(pyrPos) * glm::scale(pyrScale);

	//pyramid texture
	texFilename = "Green-plastic.jpg";
	if (!CreateTexture(texFilename, textureIdPyr)) {
		std::cout << "Failed to load texture" << texFilename << std::endl;
		return;
	}

	glUniform1i(glGetUniformLocation(shaderProgramId, "uTexture"), 0);

	//Retrieve and pass matrices to shader program
	modelLoc = glGetUniformLocation(shaderProgramId, "model");
	viewLoc = glGetUniformLocation(shaderProgramId, "view");
	projLoc = glGetUniformLocation(shaderProgramId, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//Reference matrix uniforms from the object shader for color and position
	objectColorLoc = glGetUniformLocation(shaderProgramId, "objectColor");
	//location for light
	lightColorLoc = glGetUniformLocation(shaderProgramId, "lightColor");
	lightPositionLoc = glGetUniformLocation(shaderProgramId, "lightPos");
	//Reference for fill light
	fillColorLoc = glGetUniformLocation(shaderProgramId, "fillLightColor");
	fillPosLoc = glGetUniformLocation(shaderProgramId, "fillLightPos");

	//pass info to the shader variables
	glUniform3f(objectColorLoc, pyrColor.r, pyrColor.g, pyrColor.b);
	//for main light
	glUniform3f(lightColorLoc, lampColor.r, lampColor.g, lampColor.b);
	glUniform3f(lightPositionLoc, lampPos.x, lampPos.y, lampPos.z);
	//for Fill light
	glUniform3f(fillColorLoc, fillColor.r, fillColor.g, fillColor.b);
	glUniform3f(fillPosLoc, fillPos.x, fillPos.y, fillPos.z);


	glBindVertexArray(meshPyr.vao);
	
	//Texture activation
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureIdPyr);
	
	glDrawElements(GL_TRIANGLES, meshPyr.nIndices, GL_UNSIGNED_SHORT, NULL);
	//--------------------------------------------------------------------------------------

	//set up model matrix object for the Cylinder
	//--------------------------------------------------------------------------------------
	model = glm::translate(cylPos) * glm::scale(cylScale);;

	//cylinder texture
	texFilename = "Metal-brushed.jpg";
	if (!CreateTexture(texFilename, textureIdCyl)) {
		std::cout << "Failed to load texture" << texFilename << std::endl;
		return;
	}


	glUniform1i(glGetUniformLocation(shaderProgramId, "uTexture"), 0);

	//Retrieve and pass matrices to shader program
	modelLoc = glGetUniformLocation(shaderProgramId, "model");
	viewLoc = glGetUniformLocation(shaderProgramId, "view");
	projLoc = glGetUniformLocation(shaderProgramId, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//Reference matrix uniforms from the object shader for color and position
	objectColorLoc = glGetUniformLocation(shaderProgramId, "objectColor");
	//location for light
	lightColorLoc = glGetUniformLocation(shaderProgramId, "lightColor");
	lightPositionLoc = glGetUniformLocation(shaderProgramId, "lightPos");
	//Reference for fill light
	fillColorLoc = glGetUniformLocation(shaderProgramId, "fillLightColor");
	fillPosLoc = glGetUniformLocation(shaderProgramId, "fillLightPos");

	//pass info to the shader variables
	glUniform3f(objectColorLoc, cylColor.r, cylColor.g, cylColor.b);
	//for main light
	glUniform3f(lightColorLoc, lampColor.r, lampColor.g, lampColor.b);
	glUniform3f(lightPositionLoc, lampPos.x, lampPos.y, lampPos.z);
	//for Fill light
	glUniform3f(fillColorLoc, fillColor.r, fillColor.g, fillColor.b);
	glUniform3f(fillPosLoc, fillPos.x, fillPos.y, fillPos.z);

	glBindVertexArray(meshCyl.vao);

	//Texture activation
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureIdCyl);
	
	glDrawElements(GL_TRIANGLES, meshCyl.nIndices, GL_UNSIGNED_SHORT, NULL);
	
	//Draw light: Lamp
	//--------------------------------------------------------------------------------------
	glUseProgram(lampProgramId);

	//transform light as a visual cue for the light source
	model = glm::translate(lampPos) * glm::scale(lampScale);

	//Reference matrix uniforms from lamp shader program
	modelLoc = glGetUniformLocation(lampProgramId, "model");
	viewLoc = glGetUniformLocation(lampProgramId, "view");
	projLoc = glGetUniformLocation(lampProgramId, "projection");

	//Pass matrix data to the lamp shader program's matrix
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//Use the lowest polygon object as a reference point, will also look like studio lighting rather than a random cube
	glDrawElements(GL_TRIANGLES, meshPlane.nIndices, GL_UNSIGNED_SHORT, NULL);
	//-----------------------------------------------------------------------------------------


	//Draw light: fill light
	//-----------------------------------------------------------------------------------------
	glUseProgram(fillProgramId);

	//transform light as a visual cue for light source
	model = glm::translate(fillPos) * glm::scale(fillScale);

	//Reference matrix uniforms
	modelLoc = glGetUniformLocation(fillProgramId, "model");
	viewLoc = glGetUniformLocation(fillProgramId, "view");
	projLoc = glGetUniformLocation(fillProgramId, "projection");

	//Pass matrix data to fill shader program's matrix
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glDrawElements(GL_TRIANGLES, meshPlane.nIndices, GL_UNSIGNED_SHORT, NULL);
	//------------------------------------------------------------------------------------------


	//unassign the vertex array
	glBindVertexArray(0);
	//sawp buffers and poll for input events
	glfwSwapBuffers(window);
}

//Create the mesh, stores vertices and indices and will likely need to be refactored for circles
void CreateMesh(GLMesh& mesh) {

	//Y-values for heights of each rank of the sphere
	GLfloat zTop = 0.85f;
	GLfloat zRank1 = 0.75f;
	GLfloat zRank2 = 0.5f;
	GLfloat zRank3 = 0.25f;
	GLfloat zMid = 0.0f;
	GLfloat zRank4 = -0.25f;
	GLfloat zRank5 = -0.5f;
	GLfloat zRank6 = -0.75f;
	GLfloat zBot = -1.0f;

	//Steps for circular movement
	GLfloat step1cos = cos(0 * 2 * PI / 10);
	GLfloat step1sin = sin(0 * 2 * PI / 10);
	GLfloat step2cos = cos(1 * 2 * PI / 10);
	GLfloat step2sin = sin(1 * 2 * PI / 10);
	GLfloat step3cos = cos(2 * 2 * PI / 10);
	GLfloat step3sin = sin(2 * 2 * PI / 10);
	GLfloat step4cos = cos(3 * 2 * PI / 10);
	GLfloat step4sin = sin(3 * 2 * PI / 10);
	GLfloat step5cos = cos(4 * 2 * PI / 10);
	GLfloat step5sin = sin(4 * 2 * PI / 10);
	GLfloat step6cos = cos(5 * 2 * PI / 10);
	GLfloat step6sin = sin(5 * 2 * PI / 10);
	GLfloat step7cos = cos(6 * 2 * PI / 10);
	GLfloat step7sin = sin(6 * 2 * PI / 10);
	GLfloat step8cos = cos(7 * 2 * PI / 10);
	GLfloat step8sin = sin(7 * 2 * PI / 10);
	GLfloat step9cos = cos(8 * 2 * PI / 10);
	GLfloat step9sin = sin(8 * 2 * PI / 10);
	GLfloat step10cos = cos(9 * 2 * PI / 10);
	GLfloat step10sin = sin(9 * 2 * PI / 10);


	//Radiuses to make smaller circles at ranks
	GLfloat Rank1Rad = 0.5f;
	GLfloat Rank2Rad = 0.8f;
	GLfloat Rank3Rad = 0.9f;

	GLfloat v1[] = {
		//Sphere
		//vertices					//Normals					//Texture
		0.0f, zTop, 0.0f,			0.0f, 1.0f, 0.0f,			1.0f, zTop, //top vertice, 0

		//First rank vertices
		//Feed radians in to calculate x and z coords, creates a grid for our "Squares"
		//vertices											//normals					/texture
		Rank1Rad * step1cos, zRank1, Rank1Rad * step1sin,	step1cos, zRank1, step1sin,		0.1f, zRank1, //First position, 1
		Rank1Rad * step2cos, zRank1, Rank1Rad * step2sin,	step2cos, zRank1, step2sin,		0.2f, zRank1, //Second position, 2
		Rank1Rad * step3cos, zRank1, Rank1Rad * step3sin,	step3cos, zRank1, step3sin,		0.3f, zRank1, //Third position, 3
		Rank1Rad * step4cos, zRank1, Rank1Rad * step4sin,	step4cos, zRank1, step4sin,		0.4f, zRank1, //Fourth position, 4
		Rank1Rad * step5cos, zRank1, Rank1Rad * step5sin,	step5cos, zRank1, step5sin,		0.5f, zRank1, //Fifth position, 5
		Rank1Rad * step6cos, zRank1, Rank1Rad * step6sin,	step6cos, zRank1, step6sin,		0.6f, zRank1, //Sixth Position, 6
		Rank1Rad * step7cos, zRank1, Rank1Rad * step7sin,	step7cos, zRank1, step7sin,		0.7f, zRank1, //Seventh Position, 7
		Rank1Rad * step8cos, zRank1, Rank1Rad * step8sin,	step8cos, zRank1, step8sin,		0.8f, zRank1,	//Eighth Position, 8
		Rank1Rad * step9cos, zRank1, Rank1Rad * step9sin,	step9cos, zRank1, step9sin,		0.9f, zRank1, //Ninth Position, 9
		Rank1Rad * step10cos, zRank1, Rank1Rad * step10sin,	step10cos, zRank1, step10sin,		1.0f, zRank1,//Tenth Position, 10

		//Second rank vertices
		Rank2Rad * step1cos, zRank2, Rank2Rad * step1sin,	step1cos, zRank2, step1sin,		0.1f, zRank2, //First position, 11
		Rank2Rad * step2cos, zRank2, Rank2Rad * step2sin,	step2cos, zRank2, step2sin,		0.2f, zRank2, //Second position, 12
		Rank2Rad * step3cos, zRank2, Rank2Rad * step3sin,	step3cos, zRank2, step3sin,		0.3f, zRank2, //Third position, 13
		Rank2Rad * step4cos, zRank2, Rank2Rad * step4sin,	step4cos, zRank2, step4sin,		0.4f, zRank2, //Fourth position, 14
		Rank2Rad * step5cos, zRank2, Rank2Rad * step5sin,	step5cos, zRank2, step5sin,		0.5f, zRank2, //Fifth position, 15
		Rank2Rad * step6cos, zRank2, Rank2Rad * step6sin,	step6cos, zRank2, step6sin,		0.6f, zRank2, //Sixth Position, 16
		Rank2Rad * step7cos, zRank2, Rank2Rad * step7sin,	step7cos, zRank2, step7sin,		0.7f, zRank2, //Seventh Position, 17
		Rank2Rad * step8cos, zRank2, Rank2Rad * step8sin,	step8cos, zRank2, step8sin,		0.8f, zRank2, //Eighth Position, 18
		Rank2Rad * step9cos, zRank2, Rank2Rad * step9sin,	step9cos, zRank2, step9sin,		0.9f, zRank2, //Ninth Position, 19
		Rank2Rad * step10cos, zRank2, Rank2Rad * step10sin,	step10cos, zRank2, step10sin,		1.0f, zRank2, //Tenth Position, 20

		//Third rank vertices
		Rank3Rad * step1cos, zRank3, Rank3Rad * step1sin,	step1cos, zRank3, step1sin,		0.1f, zRank3, //First position, 21
		Rank3Rad * step2cos, zRank3, Rank3Rad * step2sin,	step2cos, zRank3, step2sin,		0.2f, zRank3, //Second position, 22
		Rank3Rad * step3cos, zRank3, Rank3Rad * step3sin,	step3cos, zRank3, step3sin,		0.3f, zRank3, //Third position, 23
		Rank3Rad * step4cos, zRank3, Rank3Rad * step4sin,	step4cos, zRank3, step4sin,		0.4f, zRank3, //Fourth position, 24
		Rank3Rad * step5cos, zRank3, Rank3Rad * step5sin,	step5cos, zRank3, step5sin,		0.5f, zRank3, //Fifth position, 25
		Rank3Rad * step6cos, zRank3, Rank3Rad * step6sin,	step6cos, zRank3, step6sin,		0.6f, zRank3, //Sixth Position, 26
		Rank3Rad * step7cos, zRank3, Rank3Rad * step7sin,	step7cos, zRank3, step7sin,		0.7f, zRank3, //Seventh Position, 27
		Rank3Rad * step8cos, zRank3, Rank3Rad * step8sin,	step8cos, zRank3, step8sin,		0.8f, zRank3, //Eighth Position, 28
		Rank3Rad * step9cos, zRank3, Rank3Rad * step9sin,	step9cos, zRank3, step9sin,		0.9f, zRank3, //Ninth Position, 29
		Rank3Rad * step10cos, zRank3, Rank3Rad * step10sin,	step10cos, zRank3, step10sin,		1.0f, zRank3, //Tenth Position, 30

		//Mid rank vertices
		step1cos, zMid, step1sin,	step1cos, zMid, step1sin,		0.1f, zMid, //First position, 31
		step2cos, zMid, step2sin,	step2cos, zMid, step2sin,		0.2f, zMid, //Second position, 32
		step3cos, zMid, step3sin,	step3cos, zMid, step3sin,		0.3f, zMid, //Third position, 33
		step4cos, zMid, step4sin,	step4cos, zMid, step4sin,		0.4f, zMid, //Fourth position, 34
		step5cos, zMid, step5sin,	step5cos, zMid, step5sin,		0.5f, zMid, //Fifth position, 35
		step6cos, zMid, step6sin,	step6cos, zMid, step6sin,		0.6f, zMid, //Sixth Position, 36
		step7cos, zMid, step7sin,	step7cos, zMid, step7sin,		0.7f, zMid, //Seventh Position, 37
		step8cos, zMid, step8sin,	step8cos, zMid, step8sin,		0.8f, zMid,	//Eighth Position, 38
		step9cos, zMid, step9sin,	step9cos, zMid, step9sin,		0.9f, zMid, //Ninth Position, 39
		step10cos, zMid, step10sin,	step10cos, zMid, step10sin,		1.0f, zMid, //Tenth Position, 40

		//Fourth rank vertices
		Rank3Rad * step1cos, zRank4, Rank3Rad * step1sin,	step1cos, zRank4, step1sin,		0.1f, zRank4, //First position, 41
		Rank3Rad * step2cos, zRank4, Rank3Rad * step2sin,	step2cos, zRank4, step2sin,		0.2f, zRank4, //Second position, 42
		Rank3Rad * step3cos, zRank4, Rank3Rad * step3sin,	step3cos, zRank4, step3sin,		0.3f, zRank4, //Third position, 43
		Rank3Rad * step4cos, zRank4, Rank3Rad * step4sin,	step4cos, zRank4, step4sin,		0.4f, zRank4, //Fourth position, 44
		Rank3Rad * step5cos, zRank4, Rank3Rad * step5sin,	step5cos, zRank4, step5sin,		0.5f, zRank4, //Fifth position, 45
		Rank3Rad * step6cos, zRank4, Rank3Rad * step6sin,	step6cos, zRank4, step6sin,		0.6f, zRank4, //Sixth Position, 46
		Rank3Rad * step7cos, zRank4, Rank3Rad * step7sin,	step7cos, zRank4, step7sin,		0.7f, zRank4, //Seventh Position, 47
		Rank3Rad * step8cos, zRank4, Rank3Rad * step8sin,	step8cos, zRank4, step8sin,		0.8f, zRank4, //Eighth Position, 48
		Rank3Rad * step9cos, zRank4, Rank3Rad * step9sin,	step9cos, zRank4, step9sin,		0.9f, zRank4, //Ninth Position, 49
		Rank3Rad * step10cos, zRank4, Rank3Rad * step10sin,	step10cos, zRank4, step10sin,	1.0f, zRank4, //Tenth Position, 50

		//Fifth rank vertices
		Rank2Rad * step1cos, zRank5, Rank2Rad * step1sin,	step1cos, zRank5, step1sin,		0.1f, zRank5, //First position, 51
		Rank2Rad * step2cos, zRank5, Rank2Rad * step2sin,	step2cos, zRank5, step2sin,		0.2f, zRank5, //Second position, 52
		Rank2Rad * step3cos, zRank5, Rank2Rad * step3sin,	step3cos, zRank5, step3sin,		0.3f, zRank5, //Third position, 53
		Rank2Rad * step4cos, zRank5, Rank2Rad * step4sin,	step4cos, zRank5, step4sin,		0.4f, zRank5, //Fourth position, 54
		Rank2Rad * step5cos, zRank5, Rank2Rad * step5sin,	step5cos, zRank5, step5sin,		0.5f, zRank5, //Fifth position, 55
		Rank2Rad * step6cos, zRank5, Rank2Rad * step6sin,	step6cos, zRank5, step6sin,		0.6f, zRank5, //Sixth Position, 56
		Rank2Rad * step7cos, zRank5, Rank2Rad * step7sin,	step7cos, zRank5, step7sin,		0.7f, zRank5, //Seventh Position, 57
		Rank2Rad * step8cos, zRank5, Rank2Rad * step8sin,	step8cos, zRank5, step8sin,		0.8f, zRank5, //Eighth Position, 58
		Rank2Rad * step9cos, zRank5, Rank2Rad * step9sin,	step9cos, zRank5, step9sin,		0.9f, zRank5, //Ninth Position, 59
		Rank2Rad * step10cos, zRank5, Rank2Rad * step10sin,	step10cos, zRank5, step10sin,	1.0f, zRank5, //Tenth Position, 60

		//Sixth rank vertices
		Rank1Rad * step1cos, zRank6, Rank1Rad * step1sin,	step1cos, zRank6, step1sin,		0.1f, zRank6, //First position, 61
		Rank1Rad * step2cos, zRank6, Rank1Rad * step2sin,	step2cos, zRank6, step2sin,		0.2f, zRank6, //Second position, 62
		Rank1Rad * step3cos, zRank6, Rank1Rad * step3sin,	step3cos, zRank6, step3sin,		0.3f, zRank6, //Third position, 63
		Rank1Rad * step4cos, zRank6, Rank1Rad * step4sin,	step4cos, zRank6, step4sin,		0.4f, zRank6, //Fourth position, 64
		Rank1Rad * step5cos, zRank6, Rank1Rad * step5sin,	step5cos, zRank6, step5sin,		0.5f, zRank6, //Fifth position, 65
		Rank1Rad * step6cos, zRank6, Rank1Rad * step6sin,	step6cos, zRank6, step6sin,		0.6f, zRank6, //Sixth Position, 66
		Rank1Rad * step7cos, zRank6, Rank1Rad * step7sin,	step7cos, zRank6, step7sin,		0.7f, zRank6, //Seventh Position, 67
		Rank1Rad * step8cos, zRank6, Rank1Rad * step8sin,	step8cos, zRank6, step8sin,		0.8f, zRank6, //Eighth Position, 68
		Rank1Rad * step9cos, zRank6, Rank1Rad * step9sin,	step9cos, zRank6, step9sin,		0.9f, zRank6, //Ninth Position, 69
		Rank1Rad * step10cos, zRank6, Rank1Rad * step10sin,	step10cos, zRank6, step10sin,		1.0f, zRank6, //Tenth Position, 70

		//Bottom
		0.0f, zBot, 0.0f,			0.0f, zBot, 0.0f,		1.0f, zBot, //Bottom, 71


		//Cone
		//Don't need to interleave vertices, so will just make 10 vertices and connect to point at center of sphere
		//Top of cone
		0.0f, 0.8f, 0.0f,				0.0f, zBot, 0.0f,				1.0f, 1.0f,	//Top point, 72

		//Circle
		//Unsure of how to do scaling manually and current scale function applies to whole object
		step1cos, -3.0f, step1sin,		step1cos, 0.0f, step1sin,		0.1f, 0.0f,	//Circle position 1, 73
		step2cos, -3.0f, step2sin,		step2cos, 0.0f, step2sin,		0.2f, 0.0f,	//Circle position 2, 74
		step3cos, -3.0f, step3sin,		step3cos, 0.0f, step3sin,		0.3f, 0.0f,	//Circle position 3, 75
		step4cos, -3.0f, step4sin,		step4cos, 0.0f, step4sin,		0.4f, 0.0f,	//Circle position 4, 76
		step5cos, -3.0f, step5sin,		step5cos, 0.0f, step5sin,		0.5f, 0.0f,	//Circle position 5, 77
		step6cos, -3.0f, step6sin,		step6cos, 0.0f, step6sin,		0.6f, 0.0f,	//Circle position 6, 78
		step7cos, -3.0f, step7sin,		step7cos, 0.0f, step7sin,		0.7f, 0.0f,	//Circle position 7, 79
		step8cos, -3.0f, step8sin,		step8cos, 0.0f, step8sin,		0.8f, 0.0f,	//Circle position 8, 80
		step9cos, -3.0f, step9sin,		step9cos, 0.0f, step9sin,		0.9f, 0.0f,	//Circle position 9, 81
		step10cos, -3.0f, step10sin,	step10cos, 0.0f, step10sin,		1.0f, 0.0f,	//Circle position 10, 82

	};


	//used GLushort indices[] = {} here, might be different for spheres
	GLushort indices[] = {
		//Sphere
		//Top
		0, 1, 2,		0, 2, 3,		0, 3, 4,		0, 4, 5,		0, 5, 6,
		0, 6, 7,		0, 7, 8,		0, 8, 9,		0, 9, 10,		0, 10, 1,

		//First rank
		1, 2, 11,			2, 11, 12,		2, 3, 12,		3, 12, 13,		3, 4, 13,
		4, 13, 14,			4, 5, 14,		5, 14, 15,		5, 6, 15,		6, 15, 16,
		6, 7, 16,			7, 16, 17,		7, 8, 17,		8, 17, 18,		8, 9, 18,
		9, 18, 19,			9, 10, 19,		10, 19, 20,		10, 1, 20,		1, 20, 11,


		//Second rank
		11, 12, 21,		12, 21, 22,		12, 13, 22,		13, 22, 23,		13, 14, 23,
		14, 23, 24,		14, 15, 24,		15, 24, 25,		15, 16, 25,		16, 25, 26,
		16, 17, 26,		17, 26, 27,		17, 18, 27,		18, 27, 28,		18, 19, 28,
		19, 28, 29,		19, 20, 29,		20, 29, 30,		20, 11, 30,		11, 30, 21,

		//Third rank
		21, 22, 31,		22, 31, 32,		22, 23, 32,		23, 32, 33,		23, 24, 33,
		24, 33, 34,		24, 25, 34,		25, 34, 35,		25, 26, 35,		26, 35, 36,
		26, 27, 36,		27, 36, 37,		27, 28, 37,		28, 37, 38,		28, 29, 38,
		29, 38, 39,		29, 30, 39,		30, 39, 40,		30, 21, 40,		21, 40, 31,

		//Bottom half of sphere starts here, remove for half sphere
		//Fourth rank
		31, 32, 41,		32, 41, 42,		32, 33, 42,		33, 42, 43,		33, 34, 43,
		34, 43, 44,		34, 35, 44,		35, 44, 45,		35, 36, 45,		36, 45, 46,
		36, 37, 46,		37, 46, 47,		37, 38, 47,		38, 47, 48,		38, 39, 48,
		39, 48, 49,		39, 40, 49,		40, 49, 50,		40, 31, 50,		31, 50, 41,

		//Fifth rank
		41, 42, 51,		42, 51, 52,		42, 43, 52,		43, 52, 53,		43, 44, 53,
		44, 53, 54,		44, 45, 54,		45, 54, 55,		45, 46, 55,		46, 55, 56,
		46, 47, 56,		47, 56, 57,		47, 48, 57,		48, 57, 58,		48, 49, 58,
		49, 58, 59,		49, 50, 59,		50 ,59, 60,		50, 41, 60,		41, 60, 51,

		//Sixth rank
		51, 52, 61,		52, 61, 62,		52, 53, 62,		53, 62, 63,		53, 54, 63,
		54, 63, 64,		54, 55, 64,		55, 64, 65,		55, 56, 65,		56, 65, 66,
		56, 57, 66,		57, 66, 67,		57, 58, 67,		58, 67, 68,		58, 59, 68,
		59, 68, 69,		59, 60, 69,		60, 69, 70,		60, 51, 70,		51, 70, 61,

		//Bottom of sphere
		61, 62, 71,		62, 63, 71,		63, 64, 71,		64, 65, 71,		65, 66, 71,
		66, 67, 71,		67, 68, 71,		68, 69, 71,		69, 70, 71,		70, 61, 71,


		//Cone
		72, 73, 74,			//Cone face 1
		72, 74, 75,			//Cone face 2
		72, 75, 76,			//Cone face 3
		72, 76, 77,			//Cone face 4
		72, 77, 78,			//Cone face 5
		72, 78, 79,			//Cone face 6
		72, 79, 80,			//Cone face 7
		72, 80, 81,			//Cone face 8
		72, 81, 82,			//Cone face 9
		72, 82, 73,			//Cone face 10
		
	};

	//constants for stride
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerTexture = 2;

	//generate and bind vao
	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	//Create and bind 2 vbos for vertices and indices
	glGenBuffers(2, mesh.vbos);
	//Activate and bind buffers
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(v1), v1, GL_STATIC_DRAW);

	//Activate buffer and bind indices
	mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//establish stride
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerTexture);

	//Create attribute pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	//for texture
	glVertexAttribPointer(2, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);

	//wireframe render for debug, comment to get solid shapes
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void CreateMeshPlane(GLMesh& mesh) {
	GLfloat vertices[] = {
		//Scene plane, dark orange
		-5.0f, -3.0f, 5.0f,		0.0f, 1.0f, 0.0f,		0.0f, 1.0f,
		5.0f, -3.0f, 5.0f,		0.0f, 1.0f, 0.0f,		1.0f, 1.0f,
		-5.0f, -3.0f, -5.0f,	0.0f, 1.0f, 0.0f,		0.0f, 0.0f,
		5.0f, -3.0f, -5.0f,		0.0f, 1.0f, 0.0f,		1.0f, 0.0f,
	};

	GLushort indices[] = {
		0, 1, 2,
		1, 2, 3
	};

	//Now run it all for v2 and indices2 for the plane
	//constants for stride
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerTexture = 2;

	//generate and bind vao
	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	//Create and bind 2 vbos for vertices and indices
	glGenBuffers(2, mesh.vbos);
	//Activate and bind buffers

	//Rebind buffer and use the next set of vertices and indices
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//establish draw
	//GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor);
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerTexture);

	//Create attribute pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);
	
	//for texture
	glVertexAttribPointer(2, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);

	//wireframe render for debug, comment to get solid shapes
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void CreateMeshCube(GLMesh& mesh) {
	GLfloat vertices[] = {
		//Cube, Offwhite
		//Bottom
		-1.0f, -1.0f, 1.0f,		0.0f, -1.0f, 0.0f,		0.66f, 0.33f,		//0
		1.0f, -1.0f, 1.0f,		0.0f, -1.0f, 0.0f,		1.0f, 0.33f,		//1
		-1.0f, -1.0f, -1.0f,	0.0f, -1.0f, 0.0f,		0.66f, 0.66f,		//2
		1.0f, -1.0f, -1.0f,		0.0f, -1.0f, 0.0f,		1.0f, 0.66f,		//3

		//Front
		-1.0f, -1.0f, -1.0f,	0.0f, 0.0f, 1.0f,		0.66f, 0.0f,		//4
		1.0f, -1.0f, -1.0f,		0.0f, 0.0f, 1.0f,		1.0f, 0.0f,			//5
		-1.0f, 1.0f, -1.0f,		0.0f, 0.0f, 1.0f,		0.66f, 0.33f,		//6
		1.0f, 1.0f, -1.0f,		0.0f, 0.0f, 1.0f,		1.0f, 0.33f,		//7

		//Left
		-1.0f, -1.0f, 1.0f,		-1.0f, 0.0f, 0.0f,		0.0f, 0.33f,		//8
		-1.0f, 1.0f, 1.0f,		-1.0f, 0.0f, 0.0f,		0.0f, 0.66f,		//9
		-1.0f, 1.0f, -1.0f,		-1.0f, 0.0f, 0.0f,		0.33f, 0.66f,		//10
		-1.0f, -1.0f, -1.0f,	-1.0f, 0.0f, 0.0f,		0.33f, 0.33f,		//11

		//Back
		-1.0f, -1.0f, 1.0f,		0.0f, 0.0f, -1.0f,		0.33f, 0.0f,		//12
		-1.0f, 1.0f, 1.0f,		0.0f, 0.0f, -1.0f,		0.33f, 0.33f,		//13
		1.0f, -1.0f, 1.0f,		0.0f, 0.0f, -1.0f,		0.0f, 0.0f,			//14
		1.0f, 1.0f, 1.0f,		0.0f, 0.0f, -1.0f,		0.0f, 0.33f,		//15

		//Right
		1.0f, -1.0f, 1.0f,		1.0f, 0.0f, 0.0f,		0.66f, 0.0f,		//16
		1.0f, 1.0f, 1.0f,		1.0f, 0.0f, 0.0f,		0.66f, 0.33f,		//17
		1.0f, -1.0f, -1.0f,		1.0f, 0.0f, 0.0f,		0.33f, 0.0f,		//18
		1.0f, 1.0f, -1.0f,		1.0f, 0.0f, 0.0f,		0.33f, 0.33f,		//19

		//Top
		-1.0f, 1.0f, 1.0f,		0.0f, 1.0f, 0.0f,		0.33f, 0.66f,		//20
		1.0f, 1.0f, 1.0f,		0.0f, 1.0f, 0.0f,		0.66f, 0.66f,		//21
		-1.0f, 1.0f, -1.0f,		0.0f, 1.0f, 0.0f,		0.33f, 0.33f,		//22
		1.0f, 1.0f, -1.0f,		0.0f, 1.0f, 0.0f,		0.66f, 0.33f,		//23
	};

	GLushort indices[] = {
		0, 1, 2,	1, 2, 3,
		4, 5, 6,	5, 6, 7,
		8, 9, 10,	8, 10, 11,
		12, 13, 14,	13, 14, 15,
		16, 17, 18,	17, 18, 19,
		20, 21, 22,	21, 22, 23,
	};

	//Now run it all for v2 and indices2 for the plane
	//constants for stride
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerTexture = 2;

	//generate and bind vao
	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	//Create and bind 2 vbos for vertices and indices
	glGenBuffers(2, mesh.vbos);
	//Activate and bind buffers

	//Rebind buffer and use the next set of vertices and indices
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//establish draw
	//GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor);
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerTexture);

	//Create attribute pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);
	
	//for texture
	glVertexAttribPointer(2, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);

	//wireframe render for debug, comment to get solid shapes
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void CreateMeshPyramid(GLMesh& mesh) {
	GLfloat vertices[] = {
		//Scene Pyramid, Green
		//Front
		cos(0 * 2 * PI / 3), -0.5f, sin(0 * 2 * PI / 3),		0.0f, 0.0f, 1.0f,		0.0f, 0.0f,//0
		cos(1 * 2 * PI / 3), -0.5f, sin(1 * 2 * PI / 3),		0.0f, 0.0f, 1.0f,		1.0f, 0.0f,//1
		0.0f, 1.0f, 0.0f,										0.0f, 0.0f, 1.0f,		0.5f, 1.0f,//2

		//Left
		cos(1 * 2 * PI / 3), -0.5f, sin(1 * 2 * PI / 3),		-1.0f, 0.0f, 0.0f,		0.0f, 0.0f,//3
		cos(2 * 2 * PI / 3), -0.5f, sin(2 * 2 * PI / 3),		-1.0f, 0.0f, 0.0f,		1.0f, 0.0f,//4
		0.0f, 1.0f, 0.0f,										-1.0f, 0.0f, 0.0f,		0.5f, 1.0f,//5

		//Right
		cos(2 * 2 * PI / 3), -0.5f, sin(2 * 2 * PI / 3),		1.0f, 0.0f, 0.0f,		0.0f, 0.0f,//6
		cos(3 * 2 * PI / 3), -0.5f, sin(3 * 2 * PI / 3),		1.0f, 0.0f, 0.0f,		1.0f, 0.0f,//7
		0.0f, 1.0f, 0.0f,										1.0f, 0.0f, 0.0f,		0.5f, 1.0f,//8

		//Bottom
		cos(0 * 2 * PI / 3), -0.5f, sin(0 * 2 * PI / 3),		0.0f, -1.0f, 0.0f,		0.0f, 0.0f,//9
		cos(1 * 2 * PI / 3), -0.5f, sin(1 * 2 * PI / 3),		0.0f, -1.0f, 0.0f,		1.0f, 0.0f,//10
		cos(2 * 2 * PI / 3), -0.5f, sin(2 * 2 * PI / 3),		0.0f, -1.0f, 0.0f,		0.5f, 1.0f,//11

	};

	GLushort indices[] = {
		0, 1, 2,
		3, 4, 5,
		6, 7, 8,
		9, 10, 11,
	};

	//Now run it all for v2 and indices2 for the plane
	//constants for stride
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerTexture = 2;

	//generate and bind vao
	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	//Create and bind 2 vbos for vertices and indices
	glGenBuffers(2, mesh.vbos);
	//Activate and bind buffers

	//Rebind buffer and use the next set of vertices and indices
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//establish draw
	//GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor);
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerTexture);

	//Create attribute pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);
	
	//for texture
	glVertexAttribPointer(2, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
	
	//wireframe render for debug, comment to get solid shapes
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void CreateMeshCylinder(GLMesh& mesh) {

	GLfloat zTop = 2.0f;
	GLfloat zBot = 0.0f;

	//Steps for circular movement
	GLfloat step1cos = cos(0 * 2 * PI / 10);
	GLfloat step1sin = sin(0 * 2 * PI / 10);
	GLfloat step2cos = cos(1 * 2 * PI / 10);
	GLfloat step2sin = sin(1 * 2 * PI / 10);
	GLfloat step3cos = cos(2 * 2 * PI / 10);
	GLfloat step3sin = sin(2 * 2 * PI / 10);
	GLfloat step4cos = cos(3 * 2 * PI / 10);
	GLfloat step4sin = sin(3 * 2 * PI / 10);
	GLfloat step5cos = cos(4 * 2 * PI / 10);
	GLfloat step5sin = sin(4 * 2 * PI / 10);
	GLfloat step6cos = cos(5 * 2 * PI / 10);
	GLfloat step6sin = sin(5 * 2 * PI / 10);
	GLfloat step7cos = cos(6 * 2 * PI / 10);
	GLfloat step7sin = sin(6 * 2 * PI / 10);
	GLfloat step8cos = cos(7 * 2 * PI / 10);
	GLfloat step8sin = sin(7 * 2 * PI / 10);
	GLfloat step9cos = cos(8 * 2 * PI / 10);
	GLfloat step9sin = sin(8 * 2 * PI / 10);
	GLfloat step10cos = cos(9 * 2 * PI / 10);
	GLfloat step10sin = sin(9 * 2 * PI / 10);


	GLfloat vertices[] = {
		//Scene Cylinder, Grey
		step1cos, zTop, step1sin,		step1cos, 1.0f, step1sin,		0.0f, 1.0f,//0
		step2cos, zTop, step2sin,		step2cos, 1.0f, step2sin,		0.1f, 1.0f,//1
		step3cos, zTop, step3sin,		step3cos, 1.0f, step3sin,		0.2f, 1.0f,//2
		step4cos, zTop, step4sin,		step4cos, 1.0f, step4sin,		0.3f, 1.0f,//3
		step5cos, zTop, step5sin,		step5cos, 1.0f, step5sin,		0.4f, 1.0f,//4
		step6cos, zTop, step6sin,		step6cos, 1.0f, step6sin,		0.5f, 1.0f,//5
		step7cos, zTop, step7sin,		step7cos, 1.0f, step7sin,		0.6f, 1.0f,//6
		step8cos, zTop, step8sin,		step8cos, 1.0f, step8sin,		0.7f, 1.0f,//7
		step9cos, zTop, step9sin,		step9cos, 1.0f, step9sin,		0.8f, 1.0f,//8
		step10cos, zTop, step10sin,		step10cos, 1.0f, step10sin,		0.9f, 1.0f,//9


		step1cos, zBot, step1sin,		step1cos, -1.0f, step1sin,		0.0f, 0.0f,//10
		step2cos, zBot, step2sin,		step2cos, -1.0f, step2sin,		0.1f, 0.0f,//11
		step3cos, zBot, step3sin,		step3cos, -1.0f, step3sin,		0.2f, 0.0f,//12
		step4cos, zBot, step4sin,		step4cos, -1.0f, step4sin,		0.3f, 0.0f,//13
		step5cos, zBot, step5sin,		step5cos, -1.0f, step5sin,		0.4f, 0.0f,//14
		step6cos, zBot, step6sin,		step6cos, -1.0f, step6sin,		0.5f, 0.0f,//15
		step7cos, zBot, step7sin,		step7cos, -1.0f, step7sin,		0.6f, 0.0f,//16
		step8cos, zBot, step8sin,		step8cos, -1.0f, step8sin,		0.7f, 0.0f,//17
		step9cos, zBot, step9sin,		step9cos, -1.0f, step9sin,		0.8f, 0.0f,//18
		step10cos, zBot, step10sin,		step10cos, -1.0f, step10sin,		0.9f, 0.0f,//19


	};

	GLushort indices[] = {
		0, 1, 10,	1, 10, 11,
		1, 2, 11,	2, 11, 12,
		2, 3, 12,	3, 12, 13,
		3, 4, 13,	4, 13, 14,
		4, 5, 14,	5, 14, 15,
		5, 6, 15,	6, 15, 16,
		6, 7, 16,	7, 16, 17,
		7, 8, 17,	8, 17, 18,
		8, 9, 18,	9, 18, 19,
		9, 0, 19,	0, 19, 10,

		0, 1, 2,	0, 2, 3,
		0, 3, 4,	0, 4, 5,
		0, 5, 6,	0, 6, 7,
		0, 7, 8,	0, 8, 9,
	};

	//Now run it all for v2 and indices2 for the plane
	//constants for stride
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerTexture = 2;

	//generate and bind vao
	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	//Create and bind 2 vbos for vertices and indices
	glGenBuffers(2, mesh.vbos);
	//Activate and bind buffers

	//Rebind buffer and use the next set of vertices and indices
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//establish draw
	//GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor);
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerTexture);

	//Create attribute pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);
	
	//for texture
	glVertexAttribPointer(2, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
	
	//wireframe render for debug, comment to get solid shapes
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

//Destroy Mesh once not using
void DestroyMesh(GLMesh& mesh) {
	glDeleteVertexArrays(1, &mesh.vao);
	glDeleteBuffers(2, mesh.vbos);
}

bool CreateTexture(const char* fileName, GLuint& textureId) {
	int width, height, channels;
	unsigned char* image = stbi_load(fileName, &width, &height, &channels, 0);
	if (image) {
		flipImageVertically(image, width, height, channels);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		//texture wrapping
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		//texture filtering
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		}
		else if (channels == 4) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		}
		else {
			std::cout << "Not implemented for image with " << channels << " channels\n";
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		//unbinds the texture
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0);

		return true;
	}
	else {
		std::cout << "Texture Creation Error" << std::endl;
		return false;
	}
}

void DestroyTexture(GLuint textureId) {
	glGenTextures(1, &textureId);
}

//Create shader for vert and frag
bool CreateShaderProgram(const char* vertShaderSource,
	const char* fragShaderSource, GLuint& programId) {
	//error variables
	int success = 0;
	char infoLog[512];

	//create shader object
	programId = glCreateProgram();

	//create shaders
	GLuint vertShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	//Retrieve shader sources
	glShaderSource(vertShaderId, 1, &vertShaderSource, NULL);
	glShaderSource(fragShaderId, 1, &fragShaderSource, NULL);

	//Compile vertex shader
	glCompileShader(vertShaderId);
	//compile error check
	glGetShaderiv(vertShaderId, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
			<< infoLog << std::endl;
		return false;
	}

	//Compile fragment shader
	glCompileShader(fragShaderId);
	//error check
	glGetShaderiv(fragShaderId, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
			<< infoLog << std::endl;
		return false;
	}

	//Attach compiled shaders to program
	glAttachShader(programId, vertShaderId);
	glAttachShader(programId, fragShaderId);
	glLinkProgram(programId);
	//error check
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
			<< infoLog << std::endl;
		return false;
	}

	//execute the now-completed shader program
	glUseProgram(programId);
	return true;
}

void DestroyShaderProgram(GLuint programId) {
	glDeleteProgram(programId);
}
