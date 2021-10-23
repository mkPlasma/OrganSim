#include"solver.h"

#include<stdlib.h>
#include<math.h>
#include<algorithm>
#include"error.h"

using std::min;
using std::max;
using organSim::fatalError;
using organSim::error;


Solver::Solver(float sizeX, float sizeY, float sourceX, float sourceY, float listeningX, float listeningY) :
	stepNumber_(0), nextSampleStep_(0) {

	// Initialize domain
	domainSizeX_ = (int)round(sizeX / SIM_CELL_SIZE);
	domainSizeY_ = (int)round(sizeY / SIM_CELL_SIZE);

	for(int i = 0; i < domainSizeX_; i++)
		domain_.push_back(vector<SimCell>(domainSizeY_));


	// Set air source position / listening position
	sourceX_ = (int)round(sourceX / SIM_CELL_SIZE);
	sourceY_ = (int)round(sourceY / SIM_CELL_SIZE);
	listeningX_ = (int)round(listeningX / SIM_CELL_SIZE);
	listeningY_ = (int)round(listeningY / SIM_CELL_SIZE);

	// Check location bounds
	bool oob = false;

	if(sourceX_ < 0 || sourceX_ >= domainSizeX_){
		error("Input X is out of bounds!");
		oob = true;
	}

	if(sourceY_ < 0 || sourceY_ >= domainSizeY_){
		error("Input Y is out of bounds!");
		oob = true;
	}

	if(listeningX_ < 0 || listeningX_ >= domainSizeX_){
		error("Listening X is out of bounds!");
		oob = true;
	}

	if(listeningY_ < 0 || listeningY_ >= domainSizeY_){
		error("Listening Y is out of bounds!");
		oob = true;
	}

	if(oob)
		fatalError("Positions out of bounds!");


	// Box test model
	// TODO: remove this
	int x1 = 10;
	int x2 = 19;
	int y1 = 10;
	int y2 = 114;

	// Box walls, open on top
	for(int i = x1; i <= x2; i++){
		//domain_[i][y1].solid = true;
		domain_[i][y2 + 1].solid = true;
	}
	for(int i = y1; i <= y2; i++){
		domain_[x1][i].solid = true;
		domain_[x2][i].solid = true;
	}
	// Pipe foot
	//domain_[x1 - 1][y2].solid = true;
	//domain_[x1 - 1][y2 + 1].solid = true;
	//domain_[x1][y2 + 1].solid = true;
	//domain_[x1 + 1][y2 + 1].solid = true;

	int mouthSize = max(1, (int)round(SIM_MOUTH_SIZE / SIM_CELL_SIZE));

	for(int i = 0; i < mouthSize; i++)
		domain_[x1][y2 - 1 - i].solid = false;

	// TODO: remove this
	for(int i = 0; i < 8; i++)
		domain_[sourceX_ + i][sourceY_].excitation = true;


	// Precompute sigma prime for all cells
	float sigma = 0.5f;

	for(int i = 0; i < domainSizeX_; i++){
		for(int j = 0; j < domainSizeY_; j++){
			// Get distance to closest edge for PML
			int dist = min(i, min(j, min(domainSizeX_ - 1 - i, domainSizeY_ - 1 - j)));

			// Calculate PML sigma
			float pmlSigma = dist > SIM_PML_SIZE ? 0 : (1 - ((float)dist / SIM_PML_SIZE)) * sigma;

			// Calclate sigma term
			int beta = domain_[i][j].solid || domain_[i][j].excitation ? 0 : 1;
			domain_[i][j].sigma = 1 / (1 + (1 - beta + pmlSigma));
		}
	}

	// Initialize source sample pressure history
	sourceSampleHistory_ = vector<float>(1000);
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

		solveStep();
	}
}

// Run simulation for given number of steps
void Solver::solveSteps(int steps){
	for(int i = 0; i < steps; i++)
		solveStep();
}

// Run single simulation step
void Solver::solveStep(){

	// Simulation time in seconds
	double simTime = stepNumber_ * SIM_TIME_DELTA;

	// Get pressure at listening point to interpolate with later
	float lp = domain_[listeningX_][listeningY_].pressure;


	// Point source test
	// TODO: remove this, replace with trigger + pressure regions
	//float freq = 440;
	//double v = 6.2831853 * freq * (stepNumber_ % (int)round((1 / freq) / SIM_TIME_DELTA)) * SIM_TIME_DELTA;
	//domain_[sourceX_][sourceY_].pressure += (float)(sin(v) * 10000);


	// Calculate current pipe pressure and resulting terms
	float pressure = SIM_AIR_INPUT_PRESSURE;
	float height = 3.25e-2;

	float flueAirVelocity = sqrtf(2 * pressure / SIM_AIR_RHO);
	float tau = SIM_MOUTH_SIZE / (SIM_JET_SPEED_COEFFICIENT * flueAirVelocity);

	// Resize sample point history vector
	int newSize = max(1, min(1000, (int)round(tau / SIM_TIME_DELTA)));

	if(sourceSampleHistory_.size() != newSize)
		sourceSampleHistory_.resize(newSize, 0);

	// Update sample point history
	for(int i = 0; i < sourceSampleHistory_.size() - 1; i++)
		sourceSampleHistory_[i] = sourceSampleHistory_[i + 1];

	// TODO: change point where pressure is read to a variable offset from input point
	SimCell& sc = domain_[sourceX_ - 1][sourceY_ - 1];
	sourceSampleHistory_[sourceSampleHistory_.size() - 1] = -sc.velX;


	// Calculate values for air input
	float eta = ((sourceSampleHistory_[0]) / flueAirVelocity) * SIM_FLUE_WIDTH;
	float uBore = ((flueAirVelocity * height) / SIM_CELL_SIZE) * tanhf(eta - SIM_LABIUM_OFFSET);

	// Apply excitation
	if(isfinite(uBore))
		for(int i = 0; i < 8; i++)
			domain_[sourceX_ + i][sourceY_].velY = uBore / (SIM_CELL_SIZE * height * 8);


	for(int i = 0; i < domainSizeX_ - 1; i++){
		for(int j = 0; j < domainSizeY_ - 1; j++){

			// Get cells
			SimCell& c		= domain_[i][j];
			SimCell& cx		= domain_[i + 1][j];
			SimCell& cy		= domain_[i][j + 1];

			// Update pressure
			// Due to update pattern and zero pressure at x == 0 || y == 0, only one adjacent cell needs to be updated
			if(j > 0){
				float deriv = cx.velX + cx.velY - (domain_[i][j].velX + domain_[i + 1][j - 1].velY);
				cx.pressure = (cx.pressure - (SIM_P_TERM * deriv)) * cx.sigma;
			}

			// X velocity standard update
			if(!c.excitation && !c.solid && !cx.solid)
				c.velX = (c.velX - (SIM_V_TERM * (cx.pressure - c.pressure))) * c.sigma;

			// X velocity boundary conditions (walls)
			else{
				if(!c.solid && cx.solid)		c.velX = -abs(c.velX);
				else if(c.solid && !cx.solid)	c.velX = abs(c.velX);
			}

			// Y velocity standard update
			if(!c.excitation && !c.solid && !cy.solid)
				c.velY = (c.velY - (SIM_V_TERM * (cy.pressure - c.pressure))) * c.sigma;

			// Y velocity boundary conditions (walls)
			else{
				if(!c.solid && cy.solid)		c.velY = -abs(c.velY);
				else if(c.solid && !cy.solid)	c.velY = abs(c.velY);
			}
		}
	}

	// Add pressure value to output based on sample rate
	if(stepNumber_ >= nextSampleStep_){

		// Updated pressure
		float lpNew = domain_[listeningX_][listeningY_].pressure;

		// Push ouput value, interpolated between old and updated pressure
		output_.push_back((float)(lp + ((lpNew - lp) * (nextSampleStep_ - (int)nextSampleStep_))));

		nextSampleStep_ += SAMPLE_TIME_DELTA;
	}

	stepNumber_++;
}


const SimCell& Solver::getCell(int x, int y){
	return domain_[x][y];
}

int Solver::getDomainSizeX(){
	return domainSizeX_;
}

int Solver::getDomainSizeY(){
	return domainSizeY_;
}

int Solver::getSourceX(){
	return sourceX_;
}

int Solver::getSourceY(){
	return sourceY_;
}

int Solver::getListeningX(){
	return listeningX_;
}

int Solver::getListeningY(){
	return listeningY_;
}

const vector<float>& Solver::getOutput(){
	return output_;
}
