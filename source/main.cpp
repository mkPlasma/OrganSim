#include<iostream>
#include<AudioFile.h>
#include"solver.h"
#include"window.h"
#include"midiData.h"
#include"json.hpp"
#include"PipeManager.h"

using std::cout;
using std::endl;


//#define DEBUG_VIS
//#define DEBUG_MIDI
//#define DEBUG_JSON


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

	PipeManager manager("res/pipes.json");

#ifdef DEBUG_JSON
	cout << "Printing JSON data..." << endl;
	PipeParameters i = manager.getPipes().at(0);
	cout << i.pipeLength << endl;
	cout << "JSON data complete." << endl;
	system("pause");
	return 0;
#endif

	cout << "Starting simulation!" << endl;

	string fileName = "test";
	float secondsToSolve = 2;

	// Test pipe
	PipeParameters params = manager.getPipes().at(0);
	

	Solver solver(params);

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
	file.setNumSamplesPerChannel((int)output.size());

	// Normalize audio level, temporary
	// TODO: remove this
	float max = 0;

	for(float f : output)
		max = abs(f) > max ? abs(f) : max;

	max /= 0.5f;

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
