#include"solver.h"

#include<stdlib.h>
#include<math.h>
#include<chrono>
#include<iostream>
#include<fstream>
#include<algorithm>
#include<string>
#include"error.h"

#include<glad/glad.h>
#include<GLFW/glfw3.h>

using std::min;
using std::max;
using organSim::fatalError;
using organSim::error;

#define ACCELERATION_ENABLED

Solver::Solver(const PipeParameters& params, const vector<Note>& notes) :
	params_(params), notes_(notes), noteIndex_(0), finished_(false), sleep_(true), stepNumber_(0), nextSampleStep_(0) {

	// Get pipe size in cells
	pipeSizeX_ = (int)round(params_.pipeWidth / SIM_CELL_SIZE);
	pipeSizeY_ = (int)round(params_.pipeLength / SIM_CELL_SIZE);

	// Initialize domain
	domainSizeX_ = pipeSizeX_ + (2 * SIM_DIST_TO_PIPE) + 2;
	domainSizeY_ = pipeSizeY_ + (2 * SIM_DIST_TO_PIPE) + 3;

	// for(int i = 0; i < domainSizeX_; i++)
	// 	domain_.push_back(vector<SimCell>(domainSizeY_));

	for (int i = 0; i < domainSizeX_; i++)
		for (int j = 0; j < domainSizeY_; j++)
			domain_.push_back(SimCell());

	// Set air source position / listening position
	sourceX_ = SIM_DIST_TO_PIPE + 1;
	sourceY_ = domainSizeY_ - SIM_DIST_TO_PIPE - 2;

	listeningX_ = domainSizeX_ / 2;
	listeningY_ = SIM_PML_SIZE + (SIM_MARGIN_SIZE / 2);


	// Create pipe geometry
	for (int i = 0; i < pipeSizeX_; i++)
		domain_[get1DIndex(SIM_DIST_TO_PIPE + 1 + i, sourceY_ + 1)].solid = true;
		//domain_[SIM_DIST_TO_PIPE + 1 + i][sourceY_ + 1].solid = true;

	for(int i = 0; i < pipeSizeY_ + 3; i++){
		//domain_[SIM_DIST_TO_PIPE][SIM_DIST_TO_PIPE + i].solid = true;
		domain_[get1DIndex(SIM_DIST_TO_PIPE, SIM_DIST_TO_PIPE + 1)].solid = true;
		//domain_[domainSizeX_ - SIM_DIST_TO_PIPE - 1][SIM_DIST_TO_PIPE + i].solid = true;
		domain_[get1DIndex(domainSizeX_ - SIM_DIST_TO_PIPE - 1, SIM_DIST_TO_PIPE + i)].solid = true;
	}

	int mouthSize = max(1, (int)round(params_.mouthSize / SIM_CELL_SIZE));

	for(int i = 0; i < 1; i++)
		domain_[get1DIndex(SIM_DIST_TO_PIPE, sourceY_ - 1 - i)].solid = false;
		//domain_[SIM_DIST_TO_PIPE][sourceY_ - 1 - i].solid = false;


	// Set excitation cells
	for(int i = 0; i < pipeSizeX_; i++)
		domain_[get1DIndex(sourceX_ + i, sourceY_)].excitation = true;
		//domain_[sourceX_ + i][sourceY_].excitation = true;


	// Precompute sigma prime for all cells
	float sigma = 0.5f;

	for(int i = 0; i < domainSizeX_; i++){
		for(int j = 0; j < domainSizeY_; j++){
			// Get distance to closest edge for PML
			int dist = min(i, min(j, min(domainSizeX_ - 1 - i, domainSizeY_ - 1 - j)));

			// Calculate PML sigma
			float pmlSigma = dist > SIM_PML_SIZE ? 0 : (1 - ((float)dist / SIM_PML_SIZE)) * sigma;

			// Calclate sigma term
			int beta = domain_[get1DIndex(i, j)].solid || domain_[get1DIndex(i, j)].excitation ? 1 : 0;
			domain_[get1DIndex(i, j)].sigma = 1 / (1 + beta + pmlSigma);
		}
	}

	// Initialize source sample pressure history
	sourceSampleHistory_ = vector<float>(1000, 0);

	// Initialize uBore history
	uBoreHistory_ = vector<float>(2, 0);
	uBoreFilteredHistory_ = vector<float>(2, 0);

	// -------------------------------------------- //
	// SHADER SETUP
	// -------------------------------------------- //

	std::cout << "Creating shader..." << std::endl;
	// Require OpenGL 4.6
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Initialize GLFW and check success
	if (!glfwInit())
		std::cout << "Error initializing glfw." << std::endl;

	GLFWwindow* window_ = glfwCreateWindow(100, 100, "OrganSim", NULL, NULL);

	glfwMakeContextCurrent(window_);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		exit(-1);
	}

	// Setup shaders
	GLuint programID = glCreateProgram();

	std::cout << "Compiling shader..." << std::endl;
	GLuint shaderID = glCreateShader(GL_COMPUTE_SHADER);

	if (shaderID == 0)
		organSim::fatalError("Failed to create shader object");

	// Read in the glsl source code
	std::ifstream fileStream("C:/Users/Elijah/source/repos/OrganSimGPU/res/shaders/updatecells.comp"); // TODO don't use absolute path

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

		std::cout << "Error compiling updatecells.comp: " << std::endl;
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
}

// Run simulation until all notes have been played
void Solver::solve(){
	while(!finished_)
		solveStep();
}

// Run simulation for given number of seconds
void Solver::solveSeconds(float seconds){
	int steps = (int)round(seconds / SIM_TIME_DELTA);

	int lastProgressPrint = 0;

	for(int i = 0; i < steps; i++){

		// Print progress percentage
		int progress = (i * 100) / steps;

		if(progress >= lastProgressPrint + 5){
			printf("Progress: %d%% (%d / %d)\n", progress, i, steps);
			lastProgressPrint = progress;
		}

		solveStep(false);
	}
}

// Run simulation for given number of steps
void Solver::solveSteps(int steps){
	for(int i = 0; i < steps; i++)
		solveStep(false);
}

// Run single simulation step
void Solver::solveStep(bool useNoteData){
	// Simulation time in seconds
	double simTime = stepNumber_ * SIM_TIME_DELTA;

	// Get pressure at listening point to interpolate with later
	//float lp = domain_[listeningX_][listeningY_].pressure;
	float lp = domain_[get1DIndex(listeningX_, listeningY_)].pressure;

	float noteVolume = 1;

	if(useNoteData){
		// Get current / next note
		const Note& note = notes_[noteIndex_];

		// Increment note
		double t = simTime - note.endTimeSeconds;

		if(t > 0 && t <= SIM_TIME_DELTA){
			noteIndex_++;
			printf("  %d / %d\n", noteIndex_, (int)notes_.size());
		}


		// Calculate note volume and go to / wake from sleep
		// Note release, go to sleep
		if(noteIndex_ > 0 && simTime > notes_[noteIndex_ - 1].endTimeSeconds){
			noteVolume = 1 - (float)min(1.0, (simTime - notes_[noteIndex_ - 1].endTimeSeconds) / NOTE_RELEASE_TIME);

			// Go to sleep if enough time has passed after the last note
			if(simTime - notes_[noteIndex_ - 1].endTimeSeconds >= NOTE_RELEASE_TIME + SLEEP_DELAY_TIME){
				sleep_ = true;

				// If this was the final note, finish the simulation
				if(noteIndex_ == notes_.size())
					finished_ = true;
			}
		}

		// Note attack, wake from sleep
		if(noteIndex_ < notes_.size() && simTime > note.startTimeSeconds){
			noteVolume = (float)min(1.0, (simTime - note.startTimeSeconds) / NOTE_ATTACK_TIME);
			sleep_ = false;
		}
	}
	else
		sleep_ = false;

	// Do update
	if(!sleep_){
		updateExcitation(noteVolume);
		updateCells();
	}


	// Add pressure value to output based on sample rate
	if(stepNumber_ >= nextSampleStep_){

		// Updated pressure
		//float lpNew = domain_[listeningX_][listeningY_].pressure;
		float lpNew = domain_[get1DIndex(listeningX_, listeningY_)].pressure;

		// Push ouput value, interpolated between old and updated pressure
		output_.push_back((float)(lp + ((lpNew - lp) * (nextSampleStep_ - (int)nextSampleStep_))));

		nextSampleStep_ += SAMPLE_TIME_DELTA;
	}

	stepNumber_++;
}

void Solver::updateExcitation(float noteVolume){

	// Calculate current pipe pressure and resulting terms
	float pressure = params_.maxPressure;

	float airVelocity = sqrtf((2 * pressure) / SIM_AIR_RHO);
	float tau = params_.mouthSize / (SIM_JET_SPEED_COEFFICIENT * airVelocity);


	// Resize sample point history vector
	int newSize = max(1, min(1000, (int)round(tau / SIM_TIME_DELTA)));

	if(sourceSampleHistory_.size() != newSize)
		sourceSampleHistory_.resize(newSize, 0);


	// Update sample cell history
	for(int i = 0; i < sourceSampleHistory_.size() - 1; i++)
		sourceSampleHistory_[i] = sourceSampleHistory_[i + 1];

	// Store velocity from sample cell
	//SimCell& sc = domain_[sourceX_][sourceY_ - 1];
	SimCell& sc = domain_[get1DIndex(sourceX_, sourceY_ - 1)];
	sourceSampleHistory_[sourceSampleHistory_.size() - 1] = -sc.velX;


	// Calculate uBore for excitation
	float noise = ((rand() % 1000) / 1000.0f) * 0.25f;
	float eta = airVelocity == 0 ? 0 : ((sourceSampleHistory_[0] + noise) / airVelocity) * params_.flueWidth;
	float uBore = noteVolume * ((SIM_CORRECTION_TERM * airVelocity) / SIM_CELL_SIZE) * tanhf(eta - params_.labiumOffset);


	// Apply low pass filter
	float uBoreFiltered = (0.813244f * (uBore + (2 * uBoreHistory_[1]) + uBoreHistory_[0])) +
		(-1.591300f * uBoreFilteredHistory_[1]) + (-0.661674f * uBoreFilteredHistory_[0]);

	// Update uBore history
	uBoreHistory_[0] = uBoreHistory_[1];
	uBoreFilteredHistory_[0] = uBoreFilteredHistory_[1];
	uBoreHistory_[1] = uBore;
	uBoreFilteredHistory_[1] = uBoreFiltered;

	// Apply excitation
	if(isfinite(uBoreFiltered) && uBoreFiltered < 0)
		for(int i = 0; i < pipeSizeX_; i++)
			domain_[get1DIndex(sourceX_ + i, sourceY_)].velY = uBoreFiltered / (SIM_CELL_SIZE * pipeSizeX_);
			//domain_[sourceX_ + i][sourceY_].velY = uBoreFiltered / (SIM_CELL_SIZE * pipeSizeX_);
}

void Solver::updateCells(){
#ifdef ACCELERATION_ENABLED
	//std::cout << "here" << std::endl;
	// Buffer initialization
	GLuint ssbo = 0;

	// glGenBuffers(1, &ssbo);
	//
	// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	//
	// glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SimCell) * domain_.size(), &domain_, GL_DYNAMIC_DRAW); // todo check dynamic draw if needed

	glDispatchCompute(6, 6, 1); // todo calculate dynamically
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // probably not needed

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

	//SimCell* ptrx;

	//ptrx = (SimCell*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

	// domain_.clear();

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

#endif
#ifndef ACCELERATION_ENABLED
	// for(int i = 0; i < domainSizeX_ - 1; i++){
	// 	for(int j = 0; j < domainSizeY_ - 1; j++){
	//
	// 		// Get cells
	// 		SimCell& c		= domain_[i][j];
	// 		SimCell& cx		= domain_[i + 1][j];
	// 		SimCell& cy		= domain_[i][j + 1];
	//
	// 		// Update pressure
	// 		// Due to update pattern and zero pressure at x == 0 || y == 0, only one adjacent cell needs to be updated
	// 		if(j > 0){
	// 			float deriv = cx.velX + cx.velY - (domain_[i][j].velX + domain_[i + 1][j - 1].velY);
	// 			cx.pressure = (cx.pressure - (SIM_P_TERM * deriv)) * cx.sigma;
	// 		}
	//
	// 		// X velocity standard update
	// 		if(!c.excitation && !c.solid && !cx.solid)
	// 			c.velX = (c.velX - (SIM_V_TERM * (cx.pressure - c.pressure))) * c.sigma;
	//
	// 		// X velocity boundary conditions (walls)
	// 		else{
	// 			if(!c.solid && cx.solid)		c.velX = -abs(c.velX);
	// 			else if(c.solid && !cx.solid)	c.velX = abs(c.velX);
	// 		}
	//
	// 		// Y velocity standard update
	// 		if(!c.excitation && !c.solid && !cy.solid)
	// 			c.velY = (c.velY - (SIM_V_TERM * (cy.pressure - c.pressure))) * c.sigma;
	//
	// 		// Y velocity boundary conditions (walls)
	// 		else{
	// 			if(!c.solid && cy.solid)		c.velY = -abs(c.velY);
	// 			else if(c.solid && !cy.solid)	c.velY = abs(c.velY);
	// 		}
	// 	}
	// }
#endif
}

// void Solver::createShader()
// {
// 	// Require OpenGL 4.6
// 	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
// 	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
// 	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//
// 	// Initialize GLFW and check success
// 	if (!glfwInit())
// 		std::cout << "Error initializing glfw." << std::endl;
//
// 	GLFWwindow* window_ = glfwCreateWindow(100, 100, "OrganSim", NULL, NULL);
//
// 	glfwMakeContextCurrent(window_);
//
// 	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
// 		std::cout << "Failed to initialize OpenGL context" << std::endl;
// 		exit(-1);
// 	}
//
// 	// Setup shaders
// 	GLuint programID = glCreateProgram();
// 	GLuint shaderID = compileShader();
// 	glAttachShader(programID, shaderID);
// 	glLinkProgram(programID);
// 	glValidateProgram(programID);
// 	
//
// 	//glDetachShader(shaderID);
// 	glDeleteShader(shaderID);
//
// 	this->programID = programID;
// }
//
// GLuint Solver::compileShader()
// {
// 	GLuint shaderID = glCreateShader(GL_COMPUTE_SHADER);
//
// 	if (shaderID == 0)
// 		fatalError("Failed to create shader object");
//
// 	// Read in the glsl source code
// 	std::ifstream file("C:/Users/Elijah/source/repos/OrganSimGPU/res/shaders/updatecells.comp"); // TODO don't use absolute path
//
// 	if (file.fail())
// 		fatalError("Failed to open shader file: updatecells.comp");
//
//
// 	string fileContents = "";
// 	string line;
//
// 	while (std::getline(file, line))
// 		fileContents += line + "\n";
//
// 	file.close();
//
// 	const char* src = fileContents.c_str();
//
// 	glShaderSource(shaderID, 1, &src, NULL);
//
// 	glCompileShader(shaderID);
//
// 	// Error checking
// 	GLint success = 0;
// 	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
//
// 	if (success == GL_FALSE) {
// 		GLint maxLength = 0;
// 		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &maxLength);
//
// 		char* errorLog = new char[maxLength];
// 		glGetShaderInfoLog(shaderID, maxLength, &maxLength, errorLog);
//
// 		glDeleteShader(shaderID);
//
// 		std::cout << "Error compiling updatecells.comp: " << std::endl;
// 		fatalError(errorLog);
// 	}
//
// 	return shaderID;
// }


int Solver::getPipeSizeX() const{
	return pipeSizeX_;
}

const SimCell& Solver::getCell(int x, int y){
	return domain_[get1DIndex(x, y)];
}

int Solver::getDomainSizeX() const{
	return domainSizeX_;
}

int Solver::getDomainSizeY() const{
	return domainSizeY_;
}

int Solver::getSourceX() const{
	return sourceX_;
}

int Solver::getSourceY() const{
	return sourceY_;
}

int Solver::getListeningX() const{
	return listeningX_;
}

int Solver::getListeningY() const{
	return listeningY_;
}

float Solver::getUBore() const{
	return uBoreFilteredHistory_[1];
}

const vector<float>& Solver::getOutput() const{
	return output_;
}

SimCell* Solver::getSimCell(int x, int y)
{
	int index = x + y * domainSizeX_;
	return &domain_[index];
}

int Solver::get1DIndex(int x, int y)
{
	return x + y * domainSizeX_;
}
