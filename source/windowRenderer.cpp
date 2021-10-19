#include"window.h"

#include<iterator>

#include<iostream>
using std::cout;
using std::endl;


void Window::initRendering(){

	glClearColor(0, 0, 0, 1);

	// Initialize shader program
	shader_.compile("screen.vs", "screen.fs");
	shader_.link();
	shader_.use();

	// Render quad
	const float vertices[] = {
		0, 0,
		0, 1,
		1, 0,

		0, 1,
		1, 1,
		1, 0
	};

	// Create screen VAO
	glGenVertexArrays(1, &vao_);
	glBindVertexArray(vao_);

	// Create vertex buffer
	glGenBuffers(1, &vBuffer_);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer_);

	// Write data
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	// Create texture
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glGenTextures(1, &texture_);
	glBindTexture(GL_TEXTURE_2D, texture_);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	textureData_ = vector<GLubyte>(solver_.getDomainSizeX() * solver_.getDomainSizeY() * 3);
}

void Window::cleanupRendering(){
	shader_.destroy();
	glDeleteBuffers(1, &vBuffer_);
	glDeleteVertexArrays(1, &vao_);
	glDeleteTextures(1, &texture_);
}


void Window::render(){
	
	int domSizeX = solver_.getDomainSizeX();
	int domSizeY = solver_.getDomainSizeY();
	int lisX = solver_.getListeningX();
	int lisY = solver_.getListeningY();

	// Set texture data
	for(int x = 0; x < domSizeX; x++){
		for(int y = 0; y < domSizeX; y++){

			// Pixel index
			int i = (y * domSizeX) + x;

			// Draw listening position as green
			if(x == lisX && y == lisY){
				textureData_[i * 3]		= 0;
				textureData_[i * 3 + 1]	= 255;
				textureData_[i * 3 + 2]	= 0;
			}

			// Set air cells to color based on pressure
			else if(solver_.getCell(x, y).material.name == "air"){

				// Get pressure and clamp to -1 to 1 range
				float p = solver_.getCell(x, y).pressure;
				p = p > 1 ? 1 : p < -1 ? -1 : p;

				// High pressure - red, low pressure - blue
				if(p >= 0){
					textureData_[i * 3]		= 32 + (GLubyte)(p * 223);
					textureData_[i * 3 + 1] = 32;
					textureData_[i * 3 + 2] = 32;
				}
				else{
					textureData_[i * 3]		= 32;
					textureData_[i * 3 + 1] = 32;
					textureData_[i * 3 + 2] = 32 + (GLubyte)(-p * 223);
				}
			}

			// Set solid cells to gray
			else{
				textureData_[i * 3]		= 128;
				textureData_[i * 3 + 1]	= 128;
				textureData_[i * 3 + 2]	= 128;
			}
		}
	}

	// Write texture data
	glBindTexture(GL_TEXTURE_2D, texture_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, domSizeX, domSizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData_.data());


	// Set uniforms
	glUniform2f(shader_.getUniformLocation("windowSize"), WINDOW_SIZE_X, WINDOW_SIZE_Y);
	glUniform2f(shader_.getUniformLocation("camera"), cameraX_, cameraY_);
	glUniform1f(shader_.getUniformLocation("zoom"), zoom_);


	// Render
	glClear(GL_COLOR_BUFFER_BIT);

	glBindVertexArray(vao_);

	glEnableVertexAttribArray(0);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(0);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}
