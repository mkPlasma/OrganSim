#include"window.h"

#include<iterator>
#include<algorithm>

using std::min;
using std::max;


void Window::initRendering(){

	glClearColor(0, 0, 0, 1);

	// Initialize shader program
	shader_.compile("screen.vs", "screen.fs");
	shader_.link();
	shader_.use();

	int domSizeX = solver_.getDomainSizeX();
	int domSizeY = solver_.getDomainSizeY();
	int domSizeMax = max(domSizeX, domSizeY);
	float sizeX = ((float)domSizeX / domSizeMax) / 2;
	float sizeY = ((float)domSizeY / domSizeMax) / 2;
	float x1 = 0.5f - sizeX;
	float x2 = 0.5f + sizeX;
	float y1 = 0.5f - sizeY;
	float y2 = 0.5f + sizeY;

	// Render quad
	float vertices[] = {
		x1,		y1,
		x1,		y2,
		x2,		y1,

		x1,		y2,
		x2,		y2,
		x2,		y1
	};

	float texCoords[] = {
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

	// Create texture coordinate buffer
	glGenBuffers(1, &tcBuffer_);
	glBindBuffer(GL_ARRAY_BUFFER, tcBuffer_);

	// Write data
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), texCoords, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	// Create texture
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glGenTextures(1, &texture_);
	glBindTexture(GL_TEXTURE_2D, texture_);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	textureData_ = vector<GLubyte>(domSizeX * domSizeY * 3);
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

	// Get maximum pressure for cell coloration
	float maxPressure = 0;

	if(maxPressure == 0)
		for(int x = 0; x < domSizeX; x++)
			for(int y = 0; y < domSizeY; y++)
				if(!solver_.getCell(x, y).excitation)
					maxPressure = max(maxPressure, abs(solver_.getCell(x, y).pressure));

	// Allow some clipping to increase contrast
	maxPressure /= 4;


	// Set texture data
	for(int x = 0; x < domSizeX; x++){
		for(int y = 0; y < domSizeY; y++){

			// Pixel index
			int i = (y * domSizeX) + x;

			// Set air cells to color based on pressure
			if(!solver_.getCell(x, y).solid){

				// Get pressure and clamp to -1 to 1 range
				float p = solver_.getCell(x, y).pressure / maxPressure;
				p = max(-1.0f, min(p, 1.0f));

				// High pressure - red, low pressure - blue
				if(p >= 0){
					textureData_[i * 3]		= 16 + (GLubyte)(p * 239);
					textureData_[i * 3 + 1] = 16;
					textureData_[i * 3 + 2] = 16;
				}
				else{
					textureData_[i * 3]		= 16;
					textureData_[i * 3 + 1] = 16;
					textureData_[i * 3 + 2] = 16 + (GLubyte)(-p * 239);
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

	// Draw excitation cells as magenta
	int ind = (solver_.getSourceY() * domSizeX) + solver_.getSourceX();
	int width = solver_.getPipeSizeX();
	float u = -solver_.getUBore() / 0.0001f;
	u = min(u, 1.0f);

	for(int i = 0; i < width; i++){
		textureData_[(ind + i) * 3]		= (GLubyte)(255 * u);
		textureData_[(ind + i) * 3 + 1]	= 0;
		textureData_[(ind + i) * 3 + 2]	= (GLubyte)(255 * u);
	}

	// Draw listening position as green
	ind = (solver_.getListeningY() * domSizeX) + solver_.getListeningX();
	textureData_[ind * 3]		= 0;
	textureData_[ind * 3 + 1]	= 255;
	textureData_[ind * 3 + 2]	= 0;


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
	glEnableVertexAttribArray(1);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}
