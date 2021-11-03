#include "PipeManager.h"

#include <fstream>
#include <iostream>
#include <json.hpp>


PipeManager::PipeManager(const std::string& filePath)
{
	std::ifstream i(filePath);
	nlohmann::json models;
	i >> models;

	// Iterate through json objects
	for(auto &obj : models.items())
	{
		PipeParameters pipe;
		pipe.pipeWidth		= obj.value()["pipeWidth"];
		pipe.pipeLength		= obj.value()["pipeLength"];
		pipe.maxPressure	= obj.value()["maxPressure"];
		pipe.flueWidth		= obj.value()["flueWidth"];
		pipe.mouthSize		= obj.value()["mouthSize"];
		pipe.labiumOffset	= obj.value()["labiumOffset"];

		pipes.emplace(getNoteIndex(obj.key()), pipe);
	}
}

int PipeManager::getNoteIndex(const string& noteStr){

	int noteIndex = 0;
	string note = string(1, noteStr[0]) + (noteStr[1] == '#' ? "#" : "");
	int octave = std::stoi(noteStr.substr(note.size())) + 1;

	if(note == "C")			noteIndex = 0;
	else if(note == "C#")	noteIndex = 1;
	else if(note == "D")	noteIndex = 2;
	else if(note == "D#")	noteIndex = 3;
	else if(note == "E")	noteIndex = 4;
	else if(note == "F")	noteIndex = 5;
	else if(note == "F#")	noteIndex = 6;
	else if(note == "G")	noteIndex = 7;
	else if(note == "G#")	noteIndex = 8;
	else if(note == "A")	noteIndex = 9;
	else if(note == "A#")	noteIndex = 10;
	else if(note == "B")	noteIndex = 11;

	return (octave * 12) + noteIndex;
}

unordered_map<int, PipeParameters>& PipeManager::getPipes()
{
	return pipes;
}
