#pragma once

#include <iostream>
#include <string>
#include <stdexcept>
#include <Windows.h>
#include <tbb/task_scheduler_init.h>
#include <SoapySDR/Logger.hpp>
#include <portaudio.h>

namespace SdrBirdrec
{
	/*!
	* Initializes environment for SdrBirdrecBackend. Keep an object of this class alive as long SdrBirdrecBackend is used.
	*/
	struct SdrBirdrecEnvironment
	{
		SdrBirdrecEnvironment();
		~SdrBirdrecEnvironment();
		static void SoapySDRLogHandler(const SoapySDR::LogLevel logLevel, const char *message) { std::cout << message << std::endl; }

		tbb::task_scheduler_init init;
	};

	SdrBirdrecEnvironment::SdrBirdrecEnvironment()
	{
		if(!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
		{
			std::cout << "Process priority couldn't be set" << std::endl;
		}

		SoapySDR::registerLogHandler(SoapySDRLogHandler);

		//init portaudio
		PaError err = Pa_Initialize();
		if(err != paNoError) throw std::runtime_error("PortAudio error: " + std::string(Pa_GetErrorText(err)));
	}

	SdrBirdrecEnvironment::~SdrBirdrecEnvironment()
	{
		//terminate portaudio
		PaError err = Pa_Terminate();
		if(err != paNoError) std::cout << "PortAudio error: " + std::string(Pa_GetErrorText(err)) << std::endl;
	}
}