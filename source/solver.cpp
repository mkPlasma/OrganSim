#include"solver.h"

#include<stdlib.h>


Solver::Solver(float sizeX, float sizeY, float cellSize, float listeningX, float listeningY) :
	cellSize_(cellSize), stepNumber_(0), lastSampleTime_(-1) {

	// TODO: clean up (remove cellSize parameter?)
	cellSize_ = (float)SIM_CELL_SIZE;
	
	// Initialize materials
	matAir_ = {"air", 1.1839f, 142000};
	matSolid_ = {"solid", 1e9, 1e9};


	// Initialize domain
	domainSizeX_ = (int)round(sizeX / cellSize_);
	domainSizeY_ = (int)round(sizeY / cellSize_);

	for(int i = 0; i < domainSizeX_; i++){
		domain_.push_back(vector<SimCell>());

		for(int j = 0; j < domainSizeY_; j++)
			domain_[i].push_back(SimCell(matAir_));
	}

	// Initialize PML
	float sigma = 2;

	auto l_initPML = [sigma](vector<float>& pmlVec, int domainSize){
		for(int i = 0; i < domainSize; i++){

			// Left / Top boundary
			if(i <= SIM_PML_SIZE)
				pmlVec.push_back((1 - ((float)i / SIM_PML_SIZE)) * sigma);

			// Right / Bottom boundary
			else if(i >= domainSize - SIM_PML_SIZE)
				pmlVec.push_back((1 - ((domainSize - (float)i - 1) / SIM_PML_SIZE)) * sigma);

			// Center, no boundary
			else
				pmlVec.push_back(0);
		}
	};

	l_initPML(pmlX_, domainSizeX_);
	l_initPML(pmlY_, domainSizeY_);
	
	// Set listening position
	listeningX_ = (int)round(listeningX / cellSize_);
	listeningY_ = (int)round(listeningY / cellSize_);

	// Box test model
	// TODO: remove this
	int x1 = 50;
	int x2 = 210;
	int y1 = 125;
	int y2 = 135;
	
	// Box walls, open on left side
	for(int i = x1; i <= x2; i++){
		domain_[i][y1].material = matSolid_;
		domain_[i][y2].material = matSolid_;
	}
	for(int i = y1; i <= y2; i++){
		domain_[x1][i].material = matSolid_;
		//domain_[x2][i].material = matSolid_;
	}

	// Opening for air input
	for(int i = x1; i <= x1 + 2; i++)
		domain_[i][y1].material = matAir_;

	// Walls to isolate listening point
	for(int i = 0; i < domainSizeY_; i++){
		if(i < y1 || i > y2)
			domain_[x2][i].material = matSolid_;
	}
}


// Run simulation for given number of seconds
void Solver::solveSeconds(float seconds){
	int steps = (int)round(seconds / SIM_TIME_STEP);

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

	// Constants
	// TODO: move this to constructor
	double sc = SIM_TIME_STEP / cellSize_;
	double sc2 = 2 * sc;

	// Simulation time in seconds
	double simTime = stepNumber_ * SIM_TIME_STEP;

	// Point source test
	// TODO: remove this, replace with trigger + pressure regions
	float freq = 1600;
	float freq2 = 800;
	float freq3 = 1600;
	float m = 6.2831853f * stepNumber_ * SIM_TIME_STEP;
	domain_[51][120].pressure = (sinf(m * freq));
	//domain_[51][120].pressure = (noise_.noise(simTime * 500, 0, 0) + (noise_.noise(simTime * 1000, 0, 0) / 4)) * 5;

	// Update velocities
	for(int i = 0; i < domainSizeX_; i++){
		for(int j = 0; j < domainSizeY_; j++){

			// Get cell
			SimCell& c = domain_[i][j];

			// Don't update solid cells
			if(c.material.rho == -1)
				continue;

			// x velocity
			if(i != domainSizeX_ - 1){
				SimCell& cx = domain_[i + 1][j];
				c.velX -= (float)((sc2 / (c.material.rho + cx.material.rho)) * (cx.pressure - c.pressure));

				// PML
				c.velX -= pmlX_[i] * c.velX;
			}

			// y velocity
			if(j != domainSizeY_ - 1){
				SimCell& cy = domain_[i][j + 1];
				c.velY -= (float)((sc2 / (c.material.rho + cy.material.rho)) * (cy.pressure - c.pressure));

				// PML
				c.velY -= pmlY_[j] * c.velY;
			}
		}
	}

	// Update pressures
	for(int i = 1; i < domainSizeX_; i++){
		for(int j = 1; j < domainSizeY_; j++){

			// Get cell
			SimCell& c = domain_[i][j];

			// Don't update solid cells
			if(c.material.rho == -1)
				continue;

			c.pressure -= (float)((c.material.kappa * sc) *
				((c.velX - domain_[i - 1][j].velX) + (c.velY - domain_[i][j - 1].velY)));

			// PML
			c.pressure -= ((pmlX_[i] + pmlY_[j]) / 1) * c.pressure;
		}
	}

	// Add pressure value to output based on sample rate
	if(simTime >= lastSampleTime_ + (1.0f / SAMPLE_RATE)){
		output_.push_back(domain_[listeningX_][listeningY_].pressure);
		lastSampleTime_ = simTime;
	}

	stepNumber_++;
}


float Solver::getCellSize(){
	return cellSize_;
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

int Solver::getListeningX(){
	return listeningX_;
}

int Solver::getListeningY(){
	return listeningY_;
}

const vector<float>& Solver::getOutput(){
	return output_;
}
