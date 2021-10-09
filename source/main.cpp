#include <JuceHeader.h>
#include <iostream>
#include <stdio.h>

#define JUCE_CHECK_MEMORY_LEAKS 0

int main(int argc, char* argv[])
{
	std::cout << "Initializing audio buffer..." << std::endl;

	// Setup Audio Buffer
	int numChannels = 2;
	int numSamples = 999999;

	juce::AudioBuffer<float> buffer;
	buffer.setSize(numChannels, numSamples);

	juce::Random rand;

	for (unsigned int i = 0; i < numSamples; i++) {
		float chOne = rand.nextFloat() * 0.25f - 0.125f;
		float chTwo = rand.nextFloat() * 0.25f - 0.125f;

		buffer.addSample(0, i, chOne);
		buffer.addSample(1, i, chTwo);
	}

	std::cout << "Starting write..." << std::endl;
	// Save the wav file
	juce::File file("E:/Desktop/test.wav");
	juce::WavAudioFormat format;
	
	juce::FileOutputStream outputStream(file);

	juce::ScopedPointer<juce::AudioFormatWriter> writer(format.createWriterFor(&outputStream, 44100, 1, 16, NULL, 0));


	writer->writeFromAudioSampleBuffer(buffer, 0, 999999);

	std::cout << "Write completed." << std::endl;


    return 0;
}
