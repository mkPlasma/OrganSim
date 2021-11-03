#include<iostream>
#include<algorithm>
#include<AudioFile.h>
#include"solver.h"
#include"window.h"
#include"midiData.h"
#include"json.hpp"
#include"PipeManager.h"

using std::cout;
using std::endl;
using std::max;


#define GEOMETRY_FILE	"res/pipes.json"
#define MIDI_FILE		"res/test2.mid"
#define OUTPUT_FILE		"out"
#define DEBUG_PIPE		"D4"

//#define DEBUG_VIS
//#define DEBUG_SINGLE_PIPE
//#define DEBUG_MIDI
//#define DEBUG_JSON


int main(){
#ifdef DEBUG_MIDI
	cout << "Printing MIDI Data: " << endl;

	MidiData data("res/test.mid");

	auto& notes = data.notes;

	for(auto& i : notes)
	{
		for(auto& n : i.second)
			cout << "Note: " << i.first << "\tStart: " << n.startTimeSeconds << "\tEnd: " << n.endTimeSeconds << endl;
	}

	cout << "MIDI Data Complete." << endl;
	system("pause");
	return 0;
#endif

#ifdef DEBUG_JSON
	cout << "Printing JSON data..." << endl;
	PipeManager manager("res/pipes.json");
	PipeParameters i = manager.getPipes().at(0);
	cout << i.pipeLength << endl;
	cout << "JSON data complete." << endl;
	system("pause");
	return 0;
#endif


	cout << "Starting simulation!" << endl;

	// Load geometry data
	PipeManager manager(GEOMETRY_FILE);

#ifdef DEBUG_VIS

	Solver solver(manager.getPipes()[manager.getNoteIndex(DEBUG_PIPE)], vector<Note>());

	Window window(solver);
	window.init();
	window.run();
	return 0;
#endif


#ifndef DEBUG_SINGLE_PIPE
	// Load note data
	MidiData data(MIDI_FILE);
	auto& notes = data.notes;
#endif

#ifdef DEBUG_SINGLE_PIPE
	Solver solver(manager.getPipes()[manager.getNoteIndex(DEBUG_PIPE)], vector<Note>());
	solver.solveSeconds(1);

	vector<float> output = solver.getOutput();
#endif
#ifndef DEBUG_SINGLE_PIPE

	vector<vector<float>> outputVec;

	int index = 1;

	// Create solver for each pipe
	for(auto& i : notes){
		cout << "Progress: " << index++ << " / " << notes.size() << endl;

		Solver solver(manager.getPipes()[i.first], i.second);
		solver.solve();
		outputVec.push_back(solver.getOutput());
	}

	// Get longest output size
	int size = 0;

	for(auto v : outputVec)
		size = max(size, (int)v.size());

	vector<float> output = vector<float>(size, 0);


	// Combine outputs
	for(auto v : outputVec)
		for(int i = 0; i < v.size(); i++)
			output[i] += v[i];

#endif

	cout << "Simulation complete, saving file..." << endl;

	AudioFile<float> file;
	file.setNumChannels(2);
	file.setNumSamplesPerChannel((int)output.size());

	// Normalize audio level, temporary
	// TODO: remove this
	float max = 0;

	for(float f : output)
		max = abs(f) > max ? abs(f) : max;

	max /= 0.75f;

	// Write data
	for(int i = 0; i < output.size(); i++){
		file.samples[0][i] = output[i] / max;
		file.samples[1][i] = output[i] / max;
	}

	// Save file
	bool saved = file.save(string(OUTPUT_FILE) + ".wav", AudioFileFormat::Wave);

	// Try to save with a new name if failed
	if(!saved)
		for(int i = 1; i < 100; i++)
			if(file.save(OUTPUT_FILE + std::to_string(i) + ".wav", AudioFileFormat::Wave))
				break;
}
