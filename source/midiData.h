#pragma once

#include <string>
#include <vector>
#include <unordered_map>

using std::string;
using std::vector;
using std::unordered_map;


struct Note
{
	int channel;
	double startTimeSeconds;
	double endTimeSeconds;
	int startTick;
	int endTick;
};

class MidiData
{
public:
	MidiData(const string& filename);
	unordered_map<int, vector<Note>> notes;
};
