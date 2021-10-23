#include<iostream>
#include<AudioFile.h>
#include<MidiFile.h>
#include"solver.h"
#include"window.h"
#include"midiData.h"

using std::cout;
using std::endl;


// Domain size / listening position constants
// These should be set based on the models/user input later
#define DOMAIN_SIZE_X 1
#define DOMAIN_SIZE_Y 1
#define CELL_SIZE 0.002f

#define LISTENING_X 0.8f
#define LISTENING_Y 0.5f

#define DEBUG_VIS
//#define DEBUG_MIDI

int main(){
#ifdef DEBUG_MIDI
	cout << "Printing MIDI Data: " << endl;

	MidiData data("E:/test.mid");
	Track* track = &data.tracks.at(2); // First two tracks of the test.mid file are empty

	auto numNotes = track->numNotes;
	auto notes = track->notes;

	for(auto i = 0; i < numNotes; i++)
	{
		cout << notes[i][0] << '\t' << notes[i][1] << '\t' << notes[i][2] << endl;
	}

	cout << "MIDI Data Complete." << endl;
	system("pause");
	return 0;
#endif

	cout << "Starting simulation!" << endl;

	float secondstToSolve = 5;

	Solver solver(DOMAIN_SIZE_X, DOMAIN_SIZE_Y, CELL_SIZE, LISTENING_X, LISTENING_Y);

#ifdef DEBUG_VIS
	Window window(solver);
	window.init();
	window.run();
#endif

#ifndef DEBUG_VIS
	solver.solveSeconds(secondstToSolve);

	cout << "Simulation complete, saving file..." << endl;

	AudioFile<float> file;
	file.setNumChannels(2);
	file.setNumSamplesPerChannel(secondstToSolve * SAMPLE_RATE);

	vector<float> output = solver.getOutput();

	for(int i = 0; i < output.size(); i++){
		file.samples[0][i] = output[i];
		file.samples[1][i] = output[i];
	}

	file.save("test.wav", AudioFileFormat::Wave);
#endif
}
