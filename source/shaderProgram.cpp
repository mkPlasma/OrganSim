#include"shaderProgram.h"

#include<fstream>
#include<vector>
#include"error.h"

using organSim::fatalError;


void ShaderProgram::compile(const string& vShader, const string& fShader){

	vShader_ = compileShader(vShader, GL_VERTEX_SHADER);
	fShader_ = compileShader(fShader, GL_FRAGMENT_SHADER);

	program_ = glCreateProgram();

	glAttachShader(program_, vShader_);
	glAttachShader(program_, fShader_);

	glBindFragDataLocation(program_, 0, "fragColor");
}

GLuint ShaderProgram::compileShader(const string& path, const GLenum& type){

	GLuint shader = glCreateShader(type);

	if(shader == 0)
		fatalError("Failed to create shader object");

	// Open file in shader directory
	std::ifstream file("res/shaders/" + path);

	if(file.fail())
		fatalError("Failed to open shader file: " + path);


	string fileContents = "";
	string line;

	while(std::getline(file, line))
		fileContents += line + "\n";

	file.close();

	const char* filePtr = fileContents.c_str();
	glShaderSource(shader, 1, &filePtr, NULL);

	glCompileShader(shader);

	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if(success == GL_FALSE){
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		char* errorLog = new char[maxLength];
		glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog);

		glDeleteShader(shader);

		fatalError("Failed to compile shader: " + path + ":\n" + errorLog);
	}

	return shader;
}

void ShaderProgram::bindAttrib(int index, const string & name){
	glBindAttribLocation(program_, index, name.c_str());
}

void ShaderProgram::link(){

	glLinkProgram(program_);

	GLint success = 0;
	glGetProgramiv(program_, GL_LINK_STATUS, &success);

	if(success == GL_FALSE){
		GLint maxLength = 0;
		glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &maxLength);

		char* errorLog = new char[maxLength];
		glGetProgramInfoLog(program_, maxLength, &maxLength, errorLog);

		glDeleteProgram(program_);

		fatalError("Failed to link shader program: " + string(errorLog));
	}

	glDetachShader(program_, vShader_);
	glDetachShader(program_, fShader_);

	glDeleteShader(vShader_);
	glDeleteShader(fShader_);
}

void ShaderProgram::use(){
	glUseProgram(program_);
}

void ShaderProgram::destroy(){
	glDeleteProgram(program_);
}

GLint ShaderProgram::getUniformLocation(const GLchar* uniform){
	return glGetUniformLocation(program_, uniform);
}

GLuint ShaderProgram::getProgram(){
	return program_;
}
