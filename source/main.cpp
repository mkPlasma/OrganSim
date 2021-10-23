#include<iostream>
#include<AudioFile.h>
#include"solver.h"
#include"window.h"
#include"midiData.h"

using std::cout;
using std::endl;


// Domain size / listening position constants
// These should be set based on the models/user input later
#define DOMAIN_SIZE_X 0.12f
#define DOMAIN_SIZE_Y 0.5f

#define SOURCE_X 0.042f
#define SOURCE_Y 0.435f

#define LISTENING_X 0.02f
#define LISTENING_Y 0.05f

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

	float secondsToSolve = 5;
	string fileName = "test";

	Solver solver(DOMAIN_SIZE_X, DOMAIN_SIZE_Y, SOURCE_X, SOURCE_Y, LISTENING_X, LISTENING_Y);

#ifdef DEBUG_VIS
	Window window(solver);
	window.init();
	window.run();
#endif

#ifndef DEBUG_VIS
	solver.solveSeconds(secondsToSolve);

	cout << "Simulation complete, saving file..." << endl;

	vector<float> output = solver.getOutput();

	AudioFile<float> file;
	file.setNumChannels(2);
	file.setNumSamplesPerChannel(output.size());

	// Normalize audio level, temporary
	// TODO: remove this
	float max = 0;

	for(float f : output)
		max = abs(f) > max ? abs(f) : max;

	max /= 0.5;

	// Write data
	for(int i = 0; i < output.size(); i++){
		file.samples[0][i] = output[i] / max;
		file.samples[1][i] = output[i] / max;
	}

	// Save file
	bool saved = file.save(fileName + ".wav", AudioFileFormat::Wave);

	// Try to save with a new name if failed
	if(!saved)
		for(int i = 1; i < 100; i++)
			if(file.save(fileName + std::to_string(i) + ".wav", AudioFileFormat::Wave))
				break;
#endif
}
