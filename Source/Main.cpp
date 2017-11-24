/*
==============================================================================

This file was auto-generated!

It contains the basic startup code for a Juce application.

==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>

std::vector<float> TimeConvolution(const float * pAudioSamples, int nbAudioSamples, const float * pIrSamples, int nbIrSamples)
{
	int const n = nbAudioSamples + nbIrSamples - 1;

	std::vector<float> out(n, 0.0f);

	for (auto i(0); i < n; ++i) {
		int const jmn = (i >= nbIrSamples - 1) ? i - (nbIrSamples - 1) : 0;
		int const jmx = (i <  nbAudioSamples - 1) ? i : nbAudioSamples - 1;

		for (auto j(jmn); j <= jmx; ++j) {
			out[i] += (pAudioSamples[j] * pIrSamples[i - j]);
		}
	}

	return out;
}

//==============================================================================
int main(int argc, char* argv[])
{
	const double c_bufSampleRate = 88200;	// 88.2 kHz
	juce::Random random(time(0));

	// Prepare results file
	std::ofstream resultsFile("test_results.csv", std::ofstream::out);

	for (unsigned ulTest = 0; ulTest < 18; ++ulTest)
	{
		resultsFile << ",TIME,FREQ";
	}

	resultsFile << std::endl << "AUDIO\\IR";

	unsigned ulNbIRSamples = 2;
	for (unsigned ulTest = 0; ulTest < 18; ++ulTest)
	{
		resultsFile << "," << ulNbIRSamples << "," << ulNbIRSamples;
		ulNbIRSamples *= 2;
	}

	// Test using 2 to 65536 audio samples
	for (unsigned ulNbAudioSamples = 2; ulNbAudioSamples <= 0x7FFFF; ulNbAudioSamples *= 2)
	{
		std::cout << "ulNbAudioSamples = " << ulNbAudioSamples << std::endl;
		resultsFile << std::endl << ulNbAudioSamples;

		juce::AudioBuffer<float> audio(1, ulNbAudioSamples);

		// Populate audio buffer with random data
		float * audioArray = audio.getWritePointer(0);

		for (unsigned idx = 0; idx < ulNbAudioSamples; ++idx)
		{
			audioArray[idx] = random.nextFloat();
		}

		// Test using 2 to 65536 IR samples
		for (unsigned ulNbIRSamples = 2; ulNbIRSamples <= 0x7FFFF; ulNbIRSamples *= 2)
		{
			std::cout << "    ulNbIRSamples = " << ulNbIRSamples << std::endl;
			juce::AudioBuffer<float> ir(1, ulNbIRSamples);

			// Populate IR buffer with random data
			float * irArray = ir.getWritePointer(0);

			for (unsigned idx = 0; idx < ulNbIRSamples; ++idx)
			{
				irArray[idx] = random.nextFloat();
			}

			// Time-based (direct) convolution
			std::cout << "        TIME...";

			std::chrono::high_resolution_clock::time_point startTimeConv = std::chrono::high_resolution_clock::now();
			TimeConvolution(audio.getReadPointer(0), audio.getNumSamples(), ir.getReadPointer(0), ir.getNumSamples());
			std::chrono::high_resolution_clock::time_point stopTimeConv = std::chrono::high_resolution_clock::now();

			auto durationTimeConv = std::chrono::duration_cast<std::chrono::nanoseconds>(stopTimeConv - startTimeConv).count();

			std::cout << " [DONE]" << std::endl;

			// Prepare for FFT-based convolution
			juce::dsp::Convolution convFreq;
			convFreq.prepare({ c_bufSampleRate, std::max(ulNbAudioSamples, ulNbIRSamples), 1 });

			convFreq.copyAndLoadImpulseResponseFromBuffer(ir, c_bufSampleRate, false, false, ir.getNumSamples());

			juce::dsp::AudioBlock<float> audioBlock(audio);

			juce::AudioBuffer<float> out(1, ulNbAudioSamples + ulNbIRSamples - 1);
			juce::dsp::AudioBlock<float> outBlock(out);

			juce::dsp::ProcessContextNonReplacing<float> procCtx(audioBlock, outBlock);

			// FFT-based convolution
			std::cout << "        FREQ...";

			std::chrono::high_resolution_clock::time_point startFreqConv = std::chrono::high_resolution_clock::now();
			convFreq.process(procCtx);
			std::chrono::high_resolution_clock::time_point stopFreqConv = std::chrono::high_resolution_clock::now();

			auto durationFreqConv = std::chrono::duration_cast<std::chrono::nanoseconds>(stopFreqConv - startFreqConv).count();

			std::cout << " [DONE]" << std::endl;

			// Compare results
			std::cout << "        Winner: ";
			if (durationTimeConv < durationFreqConv)
			{
				std::cout << "TIME" << std::endl;
			}
			else if (durationTimeConv > durationFreqConv)
			{
				std::cout << "FREQ" << std::endl;
			}
			else
			{
				std::cout << "DRAW" << std::endl;
			}

			resultsFile << "," << durationTimeConv << "," << durationFreqConv;
		}
	}

	return 0;
}
