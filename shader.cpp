#include "shader.h"

Shader::Shader() { }

void Shader::init(const char* vertexPath, const char* fragmentPath) {
	// Load and compile vertex shader
	GLuint vertexName = glCreateShader(GL_VERTEX_SHADER);
	int vertexLength = 0;
	char* vertex_data = readSourceFile(vertexPath, &vertexLength);
	glShaderSource(vertexName, 1, (const char * const *)&vertex_data, &vertexLength); // 2.0
	GLint compileStatus;
	glCompileShader(vertexName); // 2.0
	glGetShaderiv(vertexName, GL_COMPILE_STATUS, &compileStatus); // 2.0
	if (!compileStatus) {
		GLint logSize = 0;
		glGetShaderiv(vertexName, GL_INFO_LOG_LENGTH, &logSize);
		char *errorLog = (char *)malloc(sizeof(char) * logSize);
		glGetShaderInfoLog(vertexName, logSize, &logSize, errorLog); // 2.0
		glDeleteShader(vertexName); // 2.0 
		printf("VERTEX ERROR %s\n", errorLog);
	}
	free(vertex_data);

	// Load and compile fragment shader
	GLuint fragmentName = glCreateShader(GL_FRAGMENT_SHADER);
	int fragmentLength = 0;
	char *fragment_data = readSourceFile(fragmentPath, &fragmentLength);
	glShaderSource(fragmentName, 1, (const char * const *)&fragment_data, &fragmentLength);
	glCompileShader(fragmentName);
	glGetShaderiv(fragmentName, GL_COMPILE_STATUS, &compileStatus);
	if (!compileStatus) {
		GLint logSize = 0;
		glGetShaderiv(fragmentName, GL_INFO_LOG_LENGTH, &logSize);
		char *errorLog = (char *)malloc(sizeof(char) * logSize);
		glGetShaderInfoLog(fragmentName, logSize, &logSize, errorLog);
		glDeleteShader(fragmentName);

		printf("FRAGMENT ERROR %s\n", errorLog);
	}
	free(fragment_data);

	// Create and link vertex program
	ID = glCreateProgram(); // 2.0
	glAttachShader(ID, vertexName); // 2.0
	glAttachShader(ID, fragmentName);
	glLinkProgram(ID); // 2.0
	GLint linkStatus;
	glGetProgramiv(ID, GL_LINK_STATUS, &linkStatus); // 2.0
	if (!linkStatus) {
		GLint logSize = 0;
		glGetProgramiv(ID, GL_INFO_LOG_LENGTH, &logSize);
		char *errorLog = (char *)malloc(sizeof(char) * logSize);
		glGetProgramInfoLog(ID, logSize, &logSize, errorLog); // 2.0

		printf("LINK ERROR %s\n", errorLog);
	}
}

void Shader::use() {

	glUseProgram(ID);
}