#include "midiData.h"

#include <iostream>
#include <map>
#include <MidiFile.h>
#include <ostream>
#include <string>
#include <vector>

struct NoteInfo
{
	int keyNumber;
	double startTimeSeconds;
	double endTimeSeconds;
	int startTick;
	int endTick;
};

MidiData::MidiData(const std::string &filename)
{
	smf::MidiFile midiFile;
	midiFile.read(filename);
	midiFile.doTimeAnalysis();
	midiFile.linkNotePairs();

	auto numTracks = midiFile.getTrackCount();
	// Iterate through tracks
	for (auto i = 0; i < numTracks; i++)
	{
		std::cout << numTracks << std::endl;
		std::vector<NoteInfo*> notes;

		// key number, NoteInfo
		std::map<double, NoteInfo*> startTimes;

		// Iterate through messages
		for (auto j = 0; j < midiFile[i].size(); j++)
		{
			auto event = midiFile[i][j];

			if (event.isNoteOn())
			{
				NoteInfo* note = new NoteInfo;
				note->keyNumber = event.getKeyNumber();
				note->startTimeSeconds = event.seconds;
				note->startTick = event.tick;
				startTimes[event.getKeyNumber()] = note;
				notes.push_back(note);
			}

			if (event.isNoteOff())
			{
				// Find start note event
				NoteInfo* note = startTimes[event.getKeyNumber()];

				note->endTimeSeconds = event.seconds;
				note->endTick = event.tick;
			}
		}

		double** notesArray = new double* [notes.size()];
		for(auto noteIndex = 0; noteIndex < notes.size(); noteIndex++)
		{
			notesArray[noteIndex] = new double[3];
			notesArray[noteIndex][0] = (double)notes.at(noteIndex)->keyNumber;
			notesArray[noteIndex][1] = notes.at(noteIndex)->startTimeSeconds;
			notesArray[noteIndex][2] = notes.at(noteIndex)->endTimeSeconds;
		}
		Track track;
		track.notes = notesArray;
		track.numNotes = notes.size();
		this->tracks.push_back(track);
		// for (auto i : notes)
		// {
		// 	std::cout << i->keyNumber << '\t' << i->startTimeSeconds << '\t' << i->endTimeSeconds << std::endl;
		// }
	}
}


MidiData::~MidiData() = default;
