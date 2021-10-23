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


// Time step / Cell size
#define SIM_TIME_DELTA 7.81e-6f
#define SIM_CELL_SIZE 3.83e-3f

// Audio sample rate
#define SAMPLE_RATE 44100
#define SAMPLE_TIME_DELTA ((1.0 / SAMPLE_RATE) / SIM_TIME_DELTA)

// Rho - Density (kg / m^3)
// Kappa - Bulk modulus (Pa)
#define SIM_AIR_RHO 1.1860f
#define SIM_AIR_KAPPA 1.42e5f

// Wave propogation speed correction term
#define SIM_SPEED_OF_SOUND 3.43e2f
#define SIM_CORRECTION_TERM (SIM_SPEED_OF_SOUND / (SIM_CELL_SIZE / SIM_TIME_DELTA))

// Precomputed terms for update
#define SIM_P_TERM	((SIM_CORRECTION_TERM * SIM_AIR_KAPPA * SIM_TIME_DELTA) / SIM_CELL_SIZE)
#define SIM_V_TERM	((SIM_CORRECTION_TERM * SIM_TIME_DELTA / SIM_CELL_SIZE) / SIM_AIR_RHO)

// Air input parameters for 440 Hz pipe
// TODO: make these variables and optimize for each pipe
#define SIM_AIR_INPUT_PRESSURE 690
#define SIM_FLUE_WIDTH 3.25e-4f
#define SIM_MOUTH_SIZE 8e-3f
#define SIM_LABIUM_OFFSET 1e-5f
#define SIM_JET_SPEED_COEFFICIENT 0.4f

// PML size in cells
#define SIM_PML_SIZE 6


// Single simulation cell containing air pressure and wave velocity
struct SimCell{
	bool solid;
	bool excitation;
	float sigma;

	float pressure;
	float velX;
	float velY;

	SimCell() : solid(false), excitation(false), sigma(0), pressure(0), velX(0), velY(0){}
};


class Solver{

	vector<vector<SimCell>> domain_;
	int domainSizeX_;
	int domainSizeY_;

	int sourceX_;
	int sourceY_;
	int listeningX_;
	int listeningY_;

	vector<float> sourceSampleHistory_;

	int stepNumber_;
	double nextSampleStep_;
	vector<float> output_;

public:
	Solver(float sizeX, float sizeY, float sourceX, float sourceY, float listeningX, float listeningY);

	// Run simulation for given number of seconds
	void solveSeconds(float seconds);

	// Run simulation for given number of steps
	void solveSteps(int steps);

	// Run single simulation step
	void solveStep();


	const SimCell& getCell(int x, int y);

	int getDomainSizeX();
	int getDomainSizeY();

	int getSourceX();
	int getSourceY();

	int getListeningX();
	int getListeningY();

	const vector<float>& getOutput();
};
