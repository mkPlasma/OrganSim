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
#include<Perlin.h>
#include "../PipeManager.h"

using std::vector;
using std::string;


// Speed of sound waves (m/s)
#define SIM_SPEED_OF_SOUND 346.13f

// Cell size / Time step
#define SIM_CELL_SIZE 3.83e-3f
#define SIM_TIME_DELTA (SIM_CELL_SIZE / (1.4142f * SIM_SPEED_OF_SOUND))

// Audio sample rate
#define SAMPLE_RATE 44100
#define SAMPLE_TIME_DELTA ((1.0 / SAMPLE_RATE) / SIM_TIME_DELTA)

// Air density (kg/m^3)
#define SIM_AIR_RHO 1.1839f

// Wave propogation speed correction term
#define SIM_CORRECTION_TERM (SIM_SPEED_OF_SOUND / (SIM_CELL_SIZE / SIM_TIME_DELTA))

// Precomputed terms for update
#define SIM_P_TERM	((SIM_CORRECTION_TERM * SIM_SPEED_OF_SOUND * SIM_SPEED_OF_SOUND * SIM_AIR_RHO * SIM_TIME_DELTA) / SIM_CELL_SIZE)
#define SIM_V_TERM	((SIM_CORRECTION_TERM * (SIM_TIME_DELTA / SIM_CELL_SIZE)) / SIM_AIR_RHO)

#define SIM_JET_SPEED_COEFFICIENT 0.4f

// PML and additional margin size in cells
#define SIM_PML_SIZE 6
#define SIM_MARGIN_SIZE 4
#define SIM_DIST_TO_PIPE (SIM_PML_SIZE + SIM_MARGIN_SIZE)


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

// Geometry parameters for solver instance
// struct PipeParameters{
// 	float pipeWidth;
// 	float pipeLength;
//
// 	float maxPressure;
// 	float flueWidth;
// 	float mouthSize;
// 	float labiumOffset;
// 	float widthParam;
// };


class Solver{

	PipeParameters params_;

	int pipeSizeX_;
	int pipeSizeY_;

	vector<vector<SimCell>> domain_;
	int domainSizeX_;
	int domainSizeY_;

	int sourceX_;
	int sourceY_;
	int listeningX_;
	int listeningY_;

	vector<float> sourceSampleHistory_;
	vector<float> uBoreHistory_;
	vector<float> uBoreFilteredHistory_;

	Perlin noise_;

	int stepNumber_;
	double nextSampleStep_;
	vector<float> output_;

public:
	Solver(PipeParameters parameters);

	// Run simulation for given number of seconds
	void solveSeconds(float seconds);

	// Run simulation for given number of steps
	void solveSteps(int steps);

	// Run single simulation step
	void solveStep();


	int getPipeSizeX() const;

	const SimCell& getCell(int x, int y);

	int getDomainSizeX() const;
	int getDomainSizeY() const;

	int getSourceX() const;
	int getSourceY() const;

	int getListeningX() const;
	int getListeningY() const;

	float getUBore() const;

	const vector<float>& getOutput() const;
};
