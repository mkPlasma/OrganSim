#pragma once

/*
 *	shaderProgram.h
 *	
 *	Author: Daniel
 *	
 *	Stores an OpenGL shader program.
 *	
 *	Programs include vertex, geometry, and fragment shader.
 *	Shader attributes can also be bound.
 *	
 */

class ShaderProgram;

#include<glad/glad.h>
#include<string>

using std::string;


class ShaderProgram{

	GLuint program_;
	GLuint vShader_, fShader_;

	GLuint compileShader(const string& path, const GLenum& type);

public:
	void compile(const string& vShader, const string& fShader);

	void bindAttrib(int index, const string& name);
	void link();

	void use();

	void destroy();

	GLint getUniformLocation(const GLchar* uniform);
	
	GLuint getProgram();
};
