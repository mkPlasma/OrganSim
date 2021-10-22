#pragma once
#include <string>
#include <vector>

struct Track
{
	double** notes;
	int numNotes;
};

class MidiData
{
public:
	MidiData(const std::string& filename);
	~MidiData();
	std::vector<Track> tracks;
};
