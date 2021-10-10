#pragma once

/*
 *	solver.h
 *	
 *	Author: Daniel
 *	
 *	Runs an FDTD simlation of sound waves and produces a list of floats at the given listening position.
 *	
 */

#include<vector>
#include<string>

using std::vector;
using std::string;


// Audio sample rate
#define SAMPLE_RATE 44100

// Time step / Cell size
#define SIM_TIME_STEP 0.00000781
#define SIM_CELL_SIZE 0.00383


// Material type for simulation cell
// Rho - Density (kg / m^3)
// Kappa - Bulk modulus (Pa)
struct SimMaterial{
	string name;
	float rho;
	float kappa;
};

// Single simulation cell containing air pressure and wave velocity
struct SimCell{
	SimMaterial material;
	float pressure;
	float velX;
	float velY;

	SimCell(SimMaterial& mat) : material(mat), pressure(0),
		velX(0), velY(0){}
};


class Solver{

	float cellSize_;

	vector<vector<SimCell>> domain_;
	int domainSizeX_;
	int domainSizeY_;

	int listeningX_;
	int listeningY_;

	SimMaterial matAir_;
	SimMaterial matSolid_;

	int stepNumber_;
	double lastSampleTime_;
	vector<float> output_;

public:
	Solver(float sizeX, float sizeY, float cellSize, float listeningX, float listeningY);

	// Run simulation for given number of seconds
	void solveSeconds(float seconds);

	// Run simulation for given number of steps
	void solve(int steps);


	float getCellSize();

	const SimCell& getCell(int x, int y);

	int getDomainSizeX();
	int getDomainSizeY();

	const vector<float>& getOutput();
};
