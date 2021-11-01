#include"solver.h"

#include<stdlib.h>
#include<math.h>
#include<algorithm>
#include"error.h"

using std::min;
using std::max;
using organSim::fatalError;
using organSim::error;


Solver::Solver(PipeParameters params) : params_(params), stepNumber_(0), nextSampleStep_(0) {

	// Get pipe size in cells
	pipeSizeX_ = (int)round(params_.pipeWidth / SIM_CELL_SIZE);
	pipeSizeY_ = (int)round(params_.pipeLength / SIM_CELL_SIZE);

	// Initialize domain
	domainSizeX_ = pipeSizeX_ + (2 * SIM_DIST_TO_PIPE) + 2;
	domainSizeY_ = pipeSizeY_ + (2 * SIM_DIST_TO_PIPE) + 3;

	for(int i = 0; i < domainSizeX_; i++)
		domain_.push_back(vector<SimCell>(domainSizeY_));


	// Set air source position / listening position
	sourceX_ = SIM_DIST_TO_PIPE + 1;
	sourceY_ = domainSizeY_ - SIM_DIST_TO_PIPE - 2;

	listeningX_ = domainSizeX_ / 2;
	listeningY_ = SIM_PML_SIZE + (SIM_MARGIN_SIZE / 2);


	// Create pipe geometry
	for(int i = 0; i < pipeSizeX_; i++)
		domain_[SIM_DIST_TO_PIPE + 1 + i][sourceY_ + 1].solid = true;

	for(int i = 0; i < pipeSizeY_ + 3; i++){
		domain_[SIM_DIST_TO_PIPE][SIM_DIST_TO_PIPE + i].solid = true;
		domain_[domainSizeX_ - SIM_DIST_TO_PIPE - 1][SIM_DIST_TO_PIPE + i].solid = true;
	}

	int mouthSize = max(1, (int)round(params_.mouthSize / SIM_CELL_SIZE));

	for(int i = 0; i < 1; i++)
		domain_[SIM_DIST_TO_PIPE][sourceY_ - 1 - i].solid = false;


	// Set excitation cells
	for(int i = 0; i < pipeSizeX_; i++)
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
			int beta = domain_[i][j].solid || domain_[i][j].excitation ? 1 : 0;
			domain_[i][j].sigma = 1 / (1 + beta + pmlSigma);
		}
	}

	// Initialize source sample pressure history
	sourceSampleHistory_ = vector<float>(1000, 0);

	// Initialize uBore history
	uBoreHistory_ = vector<float>(2, 0);
	uBoreFilteredHistory_ = vector<float>(2, 0);
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

	// TODO: remove this
	// test factor
	float f = (float)min(1.0, simTime / 10);


	// Calculate current pipe pressure and resulting terms
	float pm = (float)min(1.0, simTime / 0.02f);
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
	SimCell& sc = domain_[sourceX_][sourceY_ - 1];
	sourceSampleHistory_[sourceSampleHistory_.size() - 1] = -sc.velX;


	// Calculate uBore for excitation
	float noise = ((rand() % 1000) / 1000.0f) * 0.25f;
	float eta = airVelocity == 0 ? 0 : ((sourceSampleHistory_[0] + noise) / airVelocity) * params_.flueWidth;
	float uBore = ((SIM_CORRECTION_TERM * airVelocity) / SIM_CELL_SIZE) * tanhf(eta - params_.labiumOffset);


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
			domain_[sourceX_ + i][sourceY_].velY = uBoreFiltered / (SIM_CELL_SIZE * pipeSizeX_);


	// Update domain
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


int Solver::getPipeSizeX() const{
	return pipeSizeX_;
}

const SimCell& Solver::getCell(int x, int y){
	return domain_[x][y];
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
