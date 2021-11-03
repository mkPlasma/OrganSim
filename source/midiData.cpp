#include "midiData.h"

#include <iostream>
#include <MidiFile.h>
#include <ostream>
#include <string>
#include <vector>

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
		// Iterate through messages
		for (auto j = 0; j < midiFile[i].size(); j++)
		{
			auto event = midiFile[i][j];

			if (event.isNoteOn())
			{
				Note note;
				note.channel = event.getChannel();
				note.startTimeSeconds = event.seconds;
				note.startTick = event.tick;

				// Add note vector if not present
				int keyNum = event.getKeyNumber();

				if(notes.find(keyNum) == notes.end())
					notes.emplace(keyNum, vector<Note>());

				// Add note
				notes[keyNum].push_back(note);
			}

			else if (event.isNoteOff())
			{
				// Find start note event
				vector<Note>& vec = notes[event.getKeyNumber()];
				Note& note = vec[vec.size() - 1];

				note.endTimeSeconds = event.seconds;
				note.endTick = event.tick;
			}
		}
	}
}
