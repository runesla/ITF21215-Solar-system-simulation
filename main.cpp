#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <time.h>
#include "camera.h"
#include "shader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Vertex Buffer Identifiers
#define GLOBAL_MATRICES 0
#define MODEL_MATRIX1 1
#define MODEL_MATRIX2 2
#define LIGHT_PROPERTIES 3
#define MATERIAL_PROPERTIES 4
#define CAMERA_PROPERTIES 5
#define VERTICES 6
#define INDICES 7

// Vertex Array attributes
#define POSITION 0
#define NORMAL 1
#define UV 2

// Vertex Array binding points
#define STREAM0 0

// GLSL Uniform indices
#define TRANSFORM0 0
#define TRANSFORM1 1
#define LIGHT 2
#define MATERIAL 3
#define CAMERA 4

const unsigned int DEFAULT_WIDTH = 1024;
const unsigned int DEFAULT_HEIGHT = 768;

// Light properties (4 valued vectors due to std140 see OpenGL 4.5 reference)
GLfloat lightProperties[]{
	// Position
	0.0f, 0.0f, 0.0f, 0.0f,
	// Ambient Color
	0.0f, 0.0f, 0.0f, 0.0f,
	// Diffuse Color
	0.5f, 0.5f, 0.5f, 0.0f,
	// Specular Color
	0.6f, 0.6f, 0.6f, 0.0f
};

GLfloat materialProperties[] = {
	// Shininess color
	1.0f, 1.0f, 1.0f, 1.0f,
	// Shininess
	16.0f
};

// Uniforms values
GLfloat lightPosition[]{ 0.0f, 0.0f, 0.0f };
GLfloat lightAmbient[]{ 0.4f, 0.4f, 0.4f };
GLfloat lightDiffuse[]{ 0.9f, 0.7f, 0.7f };
GLfloat lightSpecular[]{ 0.0f, 0.0f, 0.0f };
GLfloat materialShininessColor[]{ 1.0f, 1.0f, 1.0f,  1.0f };
GLfloat materialShininess = 32.0f;
GLfloat cameraPosition[]{ 0.0f, 30.0f, 30.0f };

// Uniform locations
GLint projectionMatrixPos;
GLint viewMatrixPos;
GLint modelMatrixPos;
GLint lightPositionPos;
GLint lightAmbientPos;
GLint lightDiffusePos;
GLint lightSpecularPos;
GLint materialShininessColorPos;
GLint materialShininessPos;
GLint cameraPositionPos;

// Imagefiles
const char *textures[]{
	"planets/sun.jpg",
	"planets/mercury.png",
	"planets/venus.jpg",
	"planets/earth.jpg",
	"planets/mars.jpg",
	"planets/jupiter.jpg",
	"planets/saturn.jpg",
	"planets/uranus.jpg",
	"planets/neptune.jpg"
};
const size_t numObj = sizeof(textures) / sizeof(textures[0]);

// Cubemap textures
std::vector<std::string> cm_textures {
	"cubemap/v1/nx.png",
	"cubemap/v1/ny.png",
	"cubemap/v1/nz.png",
	"cubemap/v1/px.png",
	"cubemap/v1/py.png",
	"cubemap/v1/pz.png"
};
unsigned int cubemapTexture, skyboxVAO, skyboxVBO;

// Shaders
Shader shader, skyboxShader, textureShader;

// Skybox vertices
float skyboxVertices[] = {
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
};

// Camera
// https://learnopengl.com/Getting-started/Camera
Camera camera(glm::vec3(cameraPosition[0], cameraPosition[1], cameraPosition[2]));
float lastX = (float)DEFAULT_WIDTH / 2.0;
float lastY = (float)DEFAULT_HEIGHT / 2.0;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Names
GLuint programName;
GLuint vertexArrayName;
GLuint vertexBufferName;
//GLuint vertexBufferNames[8];
GLuint textureName[numObj];
GLuint satVertexBuf, satUVbuffer;

GLushort *indexData;
int numIndices;

// Struct for planet dimensions
typedef struct {
	float distance, orbitSpeed, size, rotationSpeed;
} planet;

planet planets[numObj] = {
	{0.0f,  0.0f,  2.0f, 0.3f },		// Sun
	{10.0f, 0.2f,  0.7f, 0.2f },		// Mercury
	{15.0f, 0.18f, 1.0f, 0.1f },		// Venus
	{20.0f, 0.14f, 1.0f, 0.3f },		// Earth
	{30.0f, 0.12f, 0.8f, 0.28f},		// Mars
	{35.0f, 0.10f, 1.7f, 0.8f },		// Jupiter
	{40.0f, 0.08f, 1.6f, 0.6f },		// Saturn
	{45.0f, 0.06f, 1.2f, 0.3f },		// Uranus
	{50.0f, 0.04f, 1.2f, 0.3f }			// Neptune
};

/*
 * Load a 2D texture from file
 */
unsigned int loadTexture(char const* filePath) {
	
	unsigned int textureID;
	glGenTextures(1, &textureID);

	// Read the texture image
	int width, height, channels;
	GLubyte *imageData = stbi_load(filePath, &width, &height, &channels, STBI_default);
	if (!imageData)
		return 0;

	// Generate a new texture name and activate it
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Set sampler properties
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	if (channels == 3)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);
	else if (channels == 4)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
	else
		return 0;

	// Generate mip map images
	glGenerateMipmap(GL_TEXTURE_2D);

	// Deactivate the texture and free the image data
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(imageData);

	return textureID;
}

/*
 * Loads a cubemap texture
 *
 * https://learnopengl.com/Advanced-OpenGL/Cubemaps
 */
unsigned int loadCubemap(std::vector<std::string> cm_textures) {

	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < cm_textures.size(); i++)
	{
		unsigned char *data = stbi_load(cm_textures[i].c_str(), &width, &height, &nrChannels, 0);
		if (data) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else {
			printf("Cubemap texture failed to load at path : %s", cm_textures[i]);
			stbi_image_free(data);
		}
	}

	// Specify cubemap wrapping and filtering methods
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

/*
 * Create a sphere with the specified radius and with the specified number of segments.
 * numV specifies the number of segments along the vertical axis
 * numH specifies the number of segments along the horizontal axis
 *
 * https://github.com/lavima/itf21215_examples/tree/master/glfw/sphere
 */
int createSphere(float radius, int numH, int numV) {

	if (numH < 4 || numV < 2)
		return 0;

	// Variables needed for the calculations
	float pi = glm::pi<float>();
	float pi2 = pi * 2.0f;
	float d1 = pi / numV;
	float d2 = pi2 / numH;

	// Allocate the data needed to store the necessary positions, normals and texture coordinates
	int numVertices = numH * (numV - 1) + 2;
	int numPer = (3 + 3 + 2);
	//GLfloat vertexData[numVertices * numPer];
	std::vector<GLfloat> vertexData(numVertices * numPer);

	// Create the top vertex
	vertexData[0] = 0.0f; vertexData[1] = radius; vertexData[2] = 0.0f;
	vertexData[3] = 0.0f; vertexData[4] = 1.0f; vertexData[5] = 0.0f;
	vertexData[6] = 0.5f; vertexData[7] = 1.0f;

	// Loop through the divisions along the vertical axis
	for (int i = 0; i < numV - 1; ++i) {
		// Loop through the divisions along the horizontal axis
		for (int j = 0; j < numH; ++j) {
			// Calculate the variables needed for this iteration
			int base = (i * numH + j + 1) * numPer;
			float t1 = d1 * (i + 1);
			float t2 = d2 * j;
			// Position (like given in lecture)
			vertexData[base] = radius * glm::sin(t2) * glm::sin(t1);
			vertexData[base + 1] = radius * glm::cos(t1);
			vertexData[base + 2] = radius * glm::cos(t2) * glm::sin(t1);
			// Normal (the same as position except unit length)
			vertexData[base + 3] = glm::sin(t2) * glm::sin(t1);
			vertexData[base + 4] = glm::cos(t1);
			vertexData[base + 5] = glm::cos(t2)*glm::sin(t1);
			// UV 
			vertexData[base + 6] = glm::asin(vertexData[base + 3]) / pi + 0.5f;
			vertexData[base + 7] = glm::asin(vertexData[base + 4]) / pi + 0.5f;
		}
	}

	// Create the bottom vertex
	vertexData[(numVertices - 1)*numPer] = 0.0f; vertexData[(numVertices - 1)*numPer + 1] = -radius; vertexData[(numVertices - 1)*numPer + 2] = 0.0f;
	vertexData[(numVertices - 1)*numPer + 3] = 0.0f; vertexData[(numVertices - 1)*numPer + 4] = -1.0f; vertexData[(numVertices - 1)*numPer + 5] = 0.0f;
	vertexData[(numVertices - 1)*numPer + 6] = 0.5f; vertexData[(numVertices - 1)*numPer + 7] = 0.0f;

	// Allocate the data needed to store the indices
	int numTriangles = (numH*(numV - 1) * 2);
	numIndices = numTriangles * 3;
	indexData = (GLushort *)malloc(numIndices * sizeof(GLushort));

	// Create the triangles for the top
	for (int j = 0; j < numH; j++) {
		indexData[j * 3] = 0;
		indexData[j * 3 + 1] = (GLushort)(j + 1);
		indexData[j * 3 + 2] = (GLushort)((j + 1) % numH + 1);
	}
	// Loop through the segment circles 
	for (int i = 0; i < numV - 2; ++i) {
		for (int j = 0; j < numH; ++j) {
			indexData[((i*numH + j) * 2 + numH) * 3] = (GLushort)(i*numH + j + 1);
			indexData[((i*numH + j) * 2 + numH) * 3 + 1] = (GLushort)((i + 1)*numH + j + 1);
			indexData[((i*numH + j) * 2 + numH) * 3 + 2] = (GLushort)((i + 1)*numH + (j + 1) % numH + 1);

			indexData[((i*numH + j) * 2 + numH) * 3 + 3] = (GLushort)((i + 1)*numH + (j + 1) % numH + 1);
			indexData[((i*numH + j) * 2 + numH) * 3 + 4] = (GLushort)(i*numH + (j + 1) % numH + 1);
			indexData[((i*numH + j) * 2 + numH) * 3 + 5] = (GLushort)(i*numH + j + 1);
		}
	}
	// Create the triangles for the bottom
	int triIndex = (numTriangles - numH);
	int vertIndex = (numV - 2)*numH + 1;
	for (short i = 0; i < numH; i++) {
		indexData[(triIndex + i) * 3] = (GLushort)(vertIndex + i);
		indexData[(triIndex + i) * 3 + 1] = (GLushort)((numH*(numV - 1) + 1));
		indexData[(triIndex + i) * 3 + 2] = (GLushort)(vertIndex + (i + 1) % numH);
	}

	glGenBuffers(1, &vertexBufferName);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferName); // 2.0
	glBufferData(GL_ARRAY_BUFFER, numVertices * numPer * sizeof(GLfloat), &vertexData[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0); // 2.0

	// Create and initialize a vertex array object
	glGenVertexArrays(1, &vertexArrayName);
	glBindVertexArray(vertexArrayName);

	// Associate vertex attributes with the binding point (POSITION NORMAL UV) and specify the format
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferName);
	glVertexAttribPointer(POSITION, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GL_FLOAT), 0); // 3.0
	glVertexAttribPointer(NORMAL, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GL_FLOAT), (void *)(3 * sizeof(GL_FLOAT))); // 3.0
	glVertexAttribPointer(UV, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GL_FLOAT), (void *)(6 * sizeof(GL_FLOAT))); // 3.0
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Enable the attributes
	glEnableVertexAttribArray(POSITION); // 2.0
	glEnableVertexAttribArray(NORMAL);
	glEnableVertexAttribArray(UV);

	glBindVertexArray(0);
	return 1;

}

/*
 * Callback function for OpenGL debug messages
 */
void glDebugCallback(GLenum sources, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *msg, const void *userParam) {
	printf("DEBUG: %s\n", msg);
}

/*
 * Initialize OpenGL
 */
int initGL() {

	// Load and compile shaders
	shader.init("shaders/default33.vert", "shaders/default33.frag");
	skyboxShader.init("shaders/cubemap.vert", "shaders/cubemap.frag");
	textureShader.init("shaders/texture.vert", "shaders/texture.frag");

	skyboxShader.use();
	skyboxShader.setInt("skybox", 0);
	textureShader.use();
	textureShader.setInt("texture1", 0);
	shader.use();
	shader.setInt("textureSampler", 0);

	// Setup skybox
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	// Load planet textures
	for (int i = 0; i < numObj; i++) {
		textureName[i] = loadTexture(textures[i]);

		if (!textureName[i]) {
			printf("Failed to load texture\n");
			return 0;
		}
	}

	// Load skybox textures
	cubemapTexture = loadCubemap(cm_textures);

	if (!cubemapTexture) {
		printf("Failed to load cubemap texture\n");
		return 0;
	}

	// Get uniform locations
	projectionMatrixPos = glGetUniformLocation(programName, "proj");
	viewMatrixPos = glGetUniformLocation(programName, "view");
	modelMatrixPos = glGetUniformLocation(programName, "model");
	lightPositionPos = glGetUniformLocation(programName, "lightPosition");
	lightAmbientPos = glGetUniformLocation(programName, "lightAmbient");
	lightDiffusePos = glGetUniformLocation(programName, "lightDiffuse");
	lightSpecularPos = glGetUniformLocation(programName, "lightSpecular");
	materialShininessColorPos = glGetUniformLocation(programName, "shininessColor");
	materialShininessPos = glGetUniformLocation(programName, "shininess");
	cameraPositionPos = glGetUniformLocation(programName, "cameraPosition");

	// Enable depth buffer testing
	glEnable(GL_DEPTH_TEST);

	return 1;

}

/*
 * Draw OpenGL screne
 */
void drawGLScene() {

	// Clear color and depth buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Activate the program
	glUseProgram(programName); // 2.0

	glm::mat4 model;

	// Change the view matrix
	glm::mat4 view = camera.GetViewMatrix();
	glUniformMatrix4fv(viewMatrixPos, 1, GL_FALSE, &view[0][0]);

	// Change the projection matrix
	glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), (float)DEFAULT_WIDTH / (float)DEFAULT_HEIGHT, 0.1f, 100.0f);
	glUniformMatrix4fv(projectionMatrixPos, 1, GL_FALSE, &proj[0][0]);
	
	shader.setMat4("model", model);
	shader.setMat4("view", view);
	shader.setMat4("proj", proj);

	glDisable(GL_DEPTH_TEST);

	// Draw skybox
	glDepthFunc(GL_LEQUAL);
	skyboxShader.use();
	view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
	skyboxShader.setMat4("view", view);
	skyboxShader.setMat4("proj", proj);
	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);

	// Enable depth buffer testing
	glEnable(GL_DEPTH_TEST);

	view = camera.GetViewMatrix();
	glUniformMatrix4fv(viewMatrixPos, 1, GL_FALSE, &view[0][0]);

	// Draw planets
	for (int i = 0; i < numObj; i++) {

		model = glm::mat4(1.0);
		model = glm::scale(model, glm::vec3(planets[i].size, planets[i].size, planets[i].size));											// Set size
		model = glm::rotate(model, (float)glfwGetTime() * planets[i].orbitSpeed, glm::vec3(0.0f, 10.0f, 0.0f));								// Set orbit
		model = glm::translate(model, glm::vec3(planets[i].distance, 0.0f, -planets[i].distance));											// Set distance
		model = glm::rotate(model, (float)glfwGetTime() * planets[i].rotationSpeed, glm::vec3(0.0f, planets[i].rotationSpeed, 0.0f));		// Set rotation
		glUniformMatrix4fv(modelMatrixPos, 1, GL_FALSE, &model[0][0]);

		
		// Bind the vertex array and texture
		glBindVertexArray(vertexArrayName);
		glBindTexture(GL_TEXTURE_2D, textureName[i]);

		textureShader.setMat4("model", model);
		textureShader.setMat4("view", view);
		textureShader.setMat4("proj", proj);

		// Draw the vertex array
		glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, indexData);
		
	}

	// Set the remaining uniforms
	glUniform3fv(lightPositionPos, 1, lightPosition);
	glUniform3f(lightAmbientPos, lightAmbient[0], lightAmbient[1], lightAmbient[2]);
	glUniform3fv(lightDiffusePos, 1, lightDiffuse);
	glUniform3fv(lightSpecularPos, 1, lightSpecular);
	glUniform4fv(materialShininessColorPos, 1, materialShininessColor);
	glUniform1f(materialShininessPos, materialShininess);
	glUniform3fv(cameraPositionPos, 1, cameraPosition);

	// Disable vertex array and texture
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Disable
	glUseProgram(0);

}

void resizeGL(int width, int height) {

	// Prevent division by zero
	if (height == 0)
		height = 1;

	// Change the projection matrix
	glm::mat4 proj = glm::perspective(3.14f / 2.0f, (float)width / height, 0.1f, 1000.0f);
	glUseProgram(programName);
	glUniformMatrix4fv(projectionMatrixPos, 1, GL_FALSE, &proj[0][0]);
	glUseProgram(0);

	// Set the OpenGL viewport
	glViewport(0, 0, width, height); // 2.0

}

/*
 * Error callback function for GLFW
 */
static void glfwErrorCallback(int error, const char* description) {
	fprintf(stderr, "Error: %s\n", description);
}

/*
 * Input event callback function for GLFW
 */
static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

/*
 * Process all movement input
 */
void processInput(GLFWwindow *window) {

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

/*
 * Window size changed callback function for GLFW
 */
void glfwWindowSizeCallback(GLFWwindow* window, int width, int height) {

	resizeGL(width, height);

}

/*
 * Mouse movement callback function
 */
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {

	if (firstMouse) {
		lastX = (float)xpos;
		lastY = (float)ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

/*
 * Program entry function
 */
int main() {

	// Set error callback
	glfwSetErrorCallback(glfwErrorCallback);

	// Initialize GLFW
	if (!glfwInit()) {
		printf("Failed to initialize GLFW\n");
		exit(EXIT_FAILURE);
	}

	// Specify minimum OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	// Create window
	GLFWwindow* window = glfwCreateWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, "Solar system", NULL, NULL);
	if (!window) {
		printf("Failed to create GLFW window\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Set input key event callback
	glfwSetKeyCallback(window, glfwKeyCallback);

	// Set window resize callback
	glfwSetWindowSizeCallback(window, glfwWindowSizeCallback);

	// Make the context current
	glfwMakeContextCurrent(window);

	// Enables cursor to remain within window
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set mouse movement callback
	glfwSetCursorPosCallback(window, mouseCallback);

	glewExperimental = GL_TRUE;

	// Initialize GLEW
	if (glewInit() != GLEW_OK) {
		printf("Failed to initialize GLEW\n");
		glfwDestroyWindow(window);
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Make GLFW swap buffers directly 
	glfwSwapInterval(0);

	// Initialize OpenGL
	if (!initGL()) {
		printf("Failed to initialize OpenGL\n");
		glfwDestroyWindow(window);
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Create sphere
	if (!createSphere(1.0f, 100, 100)) {
		printf("Failed to create sphere.\n");
		glfwDestroyWindow(window);
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Initialize OpenGL view
	resizeGL(DEFAULT_WIDTH, DEFAULT_HEIGHT);

	// Run a loop until the window is closed
	while (!glfwWindowShouldClose(window)) {

		// Per-frame time logic
		float currentFrame = (float)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Input
		processInput(window);

		// Draw OpenGL scene
		drawGLScene();

		// Swap buffers
		glfwSwapBuffers(window);

		// Poll fow input events
		glfwPollEvents();

	}
	
	// De-allocate resources
	glDeleteVertexArrays(1, &skyboxVAO);
	glDeleteBuffers(1, &skyboxVAO);


	// Shutdown GLFW
	glfwDestroyWindow(window);
	glfwTerminate();

	// Exit
	exit(EXIT_SUCCESS);

}