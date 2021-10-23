#version 330 core

uniform vec2 windowSize;
uniform vec2 camera;
uniform float zoom;

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoords;

out vec2 fTexCoords;

void main(){
	fTexCoords = texCoords;
	
	// Correct coordinates
	vec2 pos = ((position * 2.0) - 1.0);
	pos.y = -pos.y;
	
	// Apply camera position and zoom
	pos.x -= camera.x * 2;
	pos.y += camera.y * 2;
	pos *= zoom;
	
	// Convert to screen space coordinates
	pos /= windowSize;
	
	gl_Position = vec4(pos, 1.0, 1.0);
}