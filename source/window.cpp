#include"window.h"

#include<stdlib.h>
#include<stdio.h>
#include<chrono>
#include<thread>
#include"error.h"

using organSim::fatalError;


#define currentTime std::chrono::high_resolution_clock::now()
#define timeBetween(t1, t2) (int)std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count()


Window::Window(Solver& solver) : window_(nullptr), solver_(solver) {
	
}

Window::~Window(){
	glfwDestroyWindow(window_);
	glfwTerminate();
}

void Window::init(){

	// Initialize GLFW and check success
	if(!glfwInit())
		exit(EXIT_FAILURE);


	// Require OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Disable manual resizing
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	// Use raw mouse input
	if(glfwRawMouseMotionSupported())
		glfwWindowHint(GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	window_ = glfwCreateWindow(WINDOW_SIZE_X, WINDOW_SIZE_Y, "OrganSim", NULL, NULL);

	if(!window_){
		glfwTerminate();
		fatalError("Failed to create GLFW window!");
	}

	// Primary monitor
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	// Center window
	glfwSetWindowPos(window_, (mode->width - WINDOW_SIZE_X) / 2, (mode->height - WINDOW_SIZE_Y) / 2);


	glfwMakeContextCurrent(window_);

	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
		fatalError("Failed to initialize GLAD!");
	}

	glViewport(0, 0, WINDOW_SIZE_X, WINDOW_SIZE_Y);
	glfwSwapInterval(false);

	
	cameraX_ = 0;
	cameraY_ = 0;
	zoom_ = 600;

	// Set mouse wheel callback function
	glfwSetWindowUserPointer(window_, this);
	
	auto callback = [](GLFWwindow* window, double xoffset, double yoffset){
		static_cast<Window*>(glfwGetWindowUserPointer(window))->mouseScroll(yoffset);
	};

	glfwSetScrollCallback(window_, callback);

	initRendering();
}

void Window::run(){

	auto startTime = currentTime;

	while(!glfwWindowShouldClose(window_)){

		// Start time
		auto loopStartTime = currentTime;


		// Run simulation for a few steps, then render
		solver_.solve(100);
		updateMouse();
		render();
		glfwSwapBuffers(window_);


		// Poll window events
		glfwPollEvents();

		// Sleep thread to maintain constant update rate
		auto elapsed = timeBetween(loopStartTime, currentTime);

		while(timeBetween(loopStartTime, currentTime) < WIN_UPDATE_TIME_MICRO)
			std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
}

void Window::updateMouse(){

	// Mouse drag
	if(glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS){

		// Get current mouse position
		double mx, my;
		glfwGetCursorPos(window_, &mx, &my);

		// Only drag camera if mouse button was down last frame
		if(mouseDown_){
			cameraX_ -= (float)((mx - mouseX_) / zoom_);
			cameraY_ -= (float)((my - mouseY_) / zoom_);
		}

		mouseX_ = (int)mx;
		mouseY_ = (int)my;
		mouseDown_ = true;
	}
	else
		mouseDown_ = false;
}

void Window::mouseScroll(double yoffset){

	if(yoffset > 0)
		zoom_ *= 1.5;
	else if(yoffset < 0)
		zoom_ /= 1.5;

	if(zoom_ < 100)
		zoom_ = 100;
	else if(zoom_ > 2000)
		zoom_ = 2000;
}

