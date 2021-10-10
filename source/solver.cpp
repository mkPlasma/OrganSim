#include"solver.h"

#include<stdlib.h>


Solver::Solver(float sizeX, float sizeY, float cellSize, float listeningX, float listeningY) :
	cellSize_(cellSize), stepNumber_(0), lastSampleTime_(0) {

	// TODO: clean up (remove cellSize parameter?)
	cellSize_ = SIM_CELL_SIZE;
	
	// Initialize materials
	matAir_ = {"air", 1.1839f, 142000};
	matSolid_ = {"solid", 1e9, 1e9};


	// Initialize domain
	domainSizeX_ = (int)round(sizeX / cellSize_);
	domainSizeY_ = (int)round(sizeY / cellSize_);

	for(int x = 0; x < domainSizeX_; x++){
		domain_.push_back(vector<SimCell>());

		for(int y = 0; y < domainSizeY_; y++)
			domain_[x].push_back(SimCell(matAir_));
	}

	// Set listening position
	listeningX_ = (int)round(listeningX / cellSize_);
	listeningY_ = (int)round(listeningY / cellSize_);

	// Box test model
	// TODO: remove this
	int x1 = 50;
	int x2 = 210;
	int y1 = 120;
	int y2 = 140;
	
	for(int i = x1; i <= x2; i++){
		domain_[i][y1].material = matSolid_;
		domain_[i][y2].material = matSolid_;
	}
	for(int i = y1; i <= y2; i++){
		domain_[x1][i].material = matSolid_;
		//domain_[x2][i].material = matSolid_;
	}
}


// Run simulation for given number of seconds
void Solver::solveSeconds(float seconds){
	solve((int)(seconds * SAMPLE_RATE));
}

// Run simulation for given number of steps
void Solver::solve(int steps){

	// Constants
	// TODO: move this to constructor
	double sc = SIM_TIME_STEP / cellSize_;
	double sc2 = 2 * sc;

	// Point source test
	// TODO: remove this
	float freq = 1000;

	if(stepNumber_ < (1.0f / freq) / SIM_TIME_STEP){
		float x = 6.28318f * freq * stepNumber_ * SIM_TIME_STEP;
		domain_[60][130].pressure += 50 * ((1 - cosf(x)) / 2.0f) * sinf(x);
	}

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
			}

			// y velocity
			if(j != domainSizeY_ - 1){
				SimCell& cy = domain_[i][j + 1];
				c.velY -= (float)((sc2 / (c.material.rho + cy.material.rho)) * (cy.pressure - c.pressure));
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

			c.pressure -= (float)((c.material.kappa * sc)
				* ((c.velX - domain_[i - 1][j].velX) + (c.velY - domain_[i][j - 1].velY)));
		}
	}

	// Add pressure value to output based on sample rate
	double currentSimTime = stepNumber_ * SIM_TIME_STEP;

	if(currentSimTime > lastSampleTime_ + (1.0f / SAMPLE_RATE)){
		output_.push_back(domain_[listeningX_][listeningY_].pressure);
		lastSampleTime_ = currentSimTime;
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

const vector<float>& Solver::getOutput(){
	return output_;
}
