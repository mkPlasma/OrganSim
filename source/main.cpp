#include<iostream>
#include<algorithm>
#include<AudioFile.h>
#include<chrono>
#include<stdlib.h>
#include<chrono>
#include<iostream>
#include<fstream>
#include<algorithm>
#include<string>

#include "error.h"
#include"solver.h"
#include"window.h"
#include"midiData.h"
#include"json.hpp"
#include"PipeManager.h"

using std::cout;
using std::endl;
using std::max;


#define GEOMETRY_FILE	"E:/Desktop/pipes.json" // todo change back to relative paths. My VS doesn't like relative paths for some reason.
#define MIDI_FILE		"E:/Desktop/MIDI_sample.mid"
#define OUTPUT_FILE		"E:/Desktop/out"
#define DEBUG_PIPE		"D4"

//#define DEBUG_VIS
//#define DEBUG_SINGLE_PIPE
//#define DEBUG_MIDI
//#define DEBUG_JSON
//#define DEBUG_TEST_SHADER

struct ShaderData
{
	GLuint velX;
};

int main(){
	auto start = std::chrono::steady_clock::now();
#ifdef DEBUG_TEST_SHADER
	// -------------------------------------------- //
	// SHADER SETUP
	// -------------------------------------------- //

	cout << "Creating shader..." << endl;
	// Require OpenGL 4.6
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Initialize GLFW and check success
	if (!glfwInit())
		cout << "Error initializing glfw." << std::endl;

	GLFWwindow* window_ = glfwCreateWindow(100, 100, "OrganSim", NULL, NULL);

	glfwMakeContextCurrent(window_);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		cout << "Failed to initialize OpenGL context" << std::endl;
		exit(-1);
	}

	// Setup shaders
	GLuint programID = glCreateProgram();

	cout << "Compiling shader..." << endl;
	GLuint shaderID = glCreateShader(GL_COMPUTE_SHADER);

	if (shaderID == 0)
		organSim::fatalError("Failed to create shader object");

	// Read in the glsl source code
	std::ifstream fileStream("C:/Users/Elijah/source/repos/OrganSimGPU/res/shaders/test.comp"); // TODO don't use absolute path

	if (fileStream.fail())
		organSim::fatalError("Failed to open shader file: updatecells.comp");


	string fileContents = "";
	string line;

	while (std::getline(fileStream, line))
		fileContents += line + "\n";

	fileStream.close();

	const char* src = fileContents.c_str();

	glShaderSource(shaderID, 1, &src, NULL);

	glCompileShader(shaderID);

	// Error checking
	GLint success = 0;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);

	if (success == GL_FALSE) {
		GLint maxLength = 0;
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &maxLength);

		char* errorLog = new char[maxLength];
		glGetShaderInfoLog(shaderID, maxLength, &maxLength, errorLog);

		glDeleteShader(shaderID);

		cout << "Error compiling updatecells.comp: " << endl;
		organSim::fatalError(errorLog);
	}

	// -------------------------------------------- //
	// END OF SHADER SETUP
	// -------------------------------------------- //

	glAttachShader(programID, shaderID);
	glLinkProgram(programID);
	glValidateProgram(programID);

	//glDetachShader(shaderID);
	glDeleteShader(shaderID);

	glUseProgram(programID);

	vector<ShaderData> velX;
	vector<float> velY;

	ShaderData i = { 10 };

	velX.push_back(i);
	velX.push_back(i);
	velY.push_back(4);

	// Buffer initialization
	GLuint ssboVelX = 0;
	GLuint ssboVelY = 1;

	glGenBuffers(1, &ssboVelX);
	//glGenBuffers(1, &ssboVelY);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboVelX);
	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboVelY);

	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ShaderData) * velX.size(), & velX, GL_DYNAMIC_READ); // todo check dynamic draw if needed
	//glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLfloat), &velY, GL_DYNAMIC_DRAW);

	// Copy to the buffer todo check if this actually needs to be done; think this actually just updates after it's already intialized
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	// GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
	// memcpy(p, &shader_data, sizeof(shader_data));
	// glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	glDispatchCompute(1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // probably not needed

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVelX);
	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVelY);

	ShaderData* ptrx;
	//GLfloat* ptry;

	ptrx = (ShaderData *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	//ptry = (GLfloat*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

	velX.clear();
	velX.push_back(ptrx[0]);
	velX.push_back(ptrx[1]);

	//velY.clear();
	//velY.push_back(ptry[0]);

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	cout << "x: " << velX[0].velX << endl;
	cout << "x2: " << velX[1].velX << endl;
	//cout << "y: " << velY[0] << endl;

	return 0;
#endif

#ifdef DEBUG_MIDI
	cout << "Printing MIDI Data: " << endl;

	MidiData data("res/test.mid");

	auto& notes = data.notes;

	for(auto& i : notes)
	{
		for(auto& n : i.second)
			cout << "Note: " << i.first << "\tStart: " << n.startTimeSeconds << "\tEnd: " << n.endTimeSeconds << endl;
	}

	cout << "MIDI Data Complete." << endl;
	system("pause");
	return 0;
#endif

#ifdef DEBUG_JSON
	cout << "Printing JSON data..." << endl;
	PipeManager manager("res/pipes.json");
	PipeParameters i = manager.getPipes().at(0);
	cout << i.pipeLength << endl;
	cout << "JSON data complete." << endl;
	system("pause");
	return 0;
#endif


	cout << "Starting simulation!" << endl;

	// Load geometry data
	PipeManager manager(GEOMETRY_FILE);

#ifdef DEBUG_VIS

	Solver solver(manager.getPipes()[manager.getNoteIndex(DEBUG_PIPE)], vector<Note>());

	Window window(solver);
	window.init();
	window.run();
	return 0;
#endif


#ifndef DEBUG_SINGLE_PIPE
	// Load note data
	MidiData data(MIDI_FILE);
	auto& notes = data.notes;
#endif

#ifdef DEBUG_SINGLE_PIPE
	Solver solver(manager.getPipes()[manager.getNoteIndex(DEBUG_PIPE)], vector<Note>());


	// Start timer. This is the important part which we are accelerating.
	auto start = std::chrono::steady_clock::now();

	solver.solveSeconds(1);

	// Stop timer.
	auto end = std::chrono::steady_clock::now();
	cout << "Solver took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " milliseconds." << endl;

	vector<float> output = solver.getOutput();

	return 0;
#endif
#ifndef DEBUG_SINGLE_PIPE

	vector<vector<float>> outputVec;

	int index = 1;

	// Create solver for each pipe
	for(auto& i : notes){
		cout << "Progress: " << index++ << " / " << notes.size() << endl;

		Solver solver(manager.getPipes()[i.first], i.second);
		solver.solve();
		outputVec.push_back(solver.getOutput());
	}

	// Get longest output size
	int size = 0;

	for(auto v : outputVec)
		size = max(size, (int)v.size());

	vector<float> output = vector<float>(size, 0);


	// Combine outputs
	for(auto v : outputVec)
		for(int i = 0; i < v.size(); i++)
			output[i] += v[i];

#endif

	cout << "Simulation complete, saving file..." << endl;

	AudioFile<float> file;
	file.setNumChannels(2);
	file.setNumSamplesPerChannel((int)output.size());

	// Normalize audio level, temporary
	// TODO: remove this
	float max = 0;

	for(float f : output)
		max = abs(f) > max ? abs(f) : max;

	max /= 0.75f;

	// Write data
	for(int i = 0; i < output.size(); i++){
		file.samples[0][i] = output[i] / max;
		file.samples[1][i] = output[i] / max;
	}

	// Save file
	bool saved = file.save(string(OUTPUT_FILE) + ".wav", AudioFileFormat::Wave);

	// Try to save with a new name if failed
	if(!saved)
		for(int i = 1; i < 100; i++)
			if(file.save(OUTPUT_FILE + std::to_string(i) + ".wav", AudioFileFormat::Wave))
				break;
	auto end = std::chrono::steady_clock::now();
	cout << "Solver took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " milliseconds." << endl;
}
