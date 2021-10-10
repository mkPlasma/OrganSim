#pragma once

/*
 *	window.h
 *	
 *	Author: Daniel
 *	
 *	Window for renderer debugging.
 *	
 */

#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include"solver.h"
#include"shaderProgram.h"


#define WINDOW_SIZE_X 1280
#define WINDOW_SIZE_Y 720
#define WIN_UPDATES_PER_SECOND		60
#define WIN_UPDATE_TIME_MICRO		(1000000 / WIN_UPDATES_PER_SECOND)


class Window{

	Solver& solver_;
	GLFWwindow* window_;

	int mouseX_;
	int mouseY_;
	bool mouseDown_;

	float cameraX_;
	float cameraY_;
	float zoom_;

	// Rendering
	ShaderProgram shader_;

	GLuint vao_;
	GLuint vBuffer_;
	GLuint texture_;
	vector<GLubyte> textureData_;

public:
	Window(Solver& solver);
	~Window();

	// Initialize window and rendering
	// NOTE: Models must be loaded and domain set first
	void init();
	void initRendering();
	void cleanupRendering();

	// Run program in visual debug mode
	void run();

	// Debug render (in windowRender.cpp)
	void render();

	void updateMouse();
	void mouseScroll(double yoffset);
};
