#include "SdrBirdrecBackend.h"

#include <tbb/tbbmalloc_proxy.h>
#include <SoapySDR/Modules.hpp>
#include <SoapySDR/Device.hpp>
#include <NIDAQmx.h>

namespace SdrBirdrec
{
	std::vector<Kwargs> SdrBirdrecBackend::findSdrDevices(const Kwargs & args) { return SoapySDR::Device::enumerate(args); }

	std::vector<Kwargs> SdrBirdrecBackend::findAudioInputDevices()
	{
		PaError err = Pa_Initialize();
		if(err != paNoError) throw runtime_error("PortAudio error: " + string(Pa_GetErrorText(err)));

		std::vector<Kwargs> list;

		int numDevices = Pa_GetDeviceCount();
		int defaultDevice = Pa_GetDefaultInputDevice();
		const PaDeviceInfo *deviceInfo;
		const PaHostApiInfo* hostApiInfo;
		for (int i = 0; i<numDevices; i++)
		{
			deviceInfo = Pa_GetDeviceInfo(i);
			hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
			if (deviceInfo->maxInputChannels == 0) continue;

			Kwargs device;
			device["deviceIndex"s] = to_string(i);
			device["name"s] = deviceInfo->name;
			device["hostApiName"s] = hostApiInfo->name;
			device["maxInputChannels"s] = to_string(deviceInfo->maxInputChannels);
			device["defaultLowInputLatency"s] = to_string(deviceInfo->defaultLowInputLatency*1e3) + "ms"s;
			device["defaultHighInputLatency"s] = to_string(deviceInfo->defaultHighInputLatency*1e3) + "ms"s;
			device["defaultSampleRate"s] = to_string(deviceInfo->defaultSampleRate) + "Hz"s;
			device["defaultDevice"s] = to_string(i == defaultDevice);
			list.push_back(device);
		}

		err = Pa_Terminate();
		if(err != paNoError) cout << "PortAudio error: " + string(Pa_GetErrorText(err)) << endl;

		return list;
	}

	std::vector<Kwargs> SdrBirdrecBackend::findAudioOutputDevices()
	{
		PaError err = Pa_Initialize();
		if(err != paNoError) throw runtime_error("PortAudio error: " + string(Pa_GetErrorText(err)));

		std::vector<Kwargs> list;

		int numDevices = Pa_GetDeviceCount();
		int defaultDevice = Pa_GetDefaultOutputDevice();
		const PaDeviceInfo *deviceInfo;
		const PaHostApiInfo* hostApiInfo;
		for (int i = 0; i<numDevices; i++)
		{
			deviceInfo = Pa_GetDeviceInfo(i);
			hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
			if (deviceInfo->maxOutputChannels == 0) continue;

			Kwargs device;
			device["deviceIndex"s] = to_string(i);
			device["name"s] = deviceInfo->name;
			device["hostApiName"s] = hostApiInfo->name;
			device["maxOutputChannels"s] = to_string(deviceInfo->maxOutputChannels);
			device["defaultLowOutputLatency"s] = to_string(deviceInfo->defaultLowOutputLatency*1e3) + "ms"s;
			device["defaultHighOutputLatency"s] = to_string(deviceInfo->defaultHighOutputLatency*1e3) + "ms"s;
			device["defaultSampleRate"s] = to_string(deviceInfo->defaultSampleRate) + "Hz"s;
			device["defaultDevice"s] = to_string(i == defaultDevice);
			list.push_back(device);
		}

		err = Pa_Terminate();
		if(err != paNoError) cout << "PortAudio error: " + string(Pa_GetErrorText(err)) << endl;

		return list;
	}

	std::vector<Kwargs> SdrBirdrecBackend::findNIDAQmxDevices()
	{
		std::vector<Kwargs> list;
		int bufferSize = DAQmxGetSysDevNames(NULL, 0);
		char *names = new char[bufferSize];
		DAQmxGetSysDevNames(names, bufferSize);
		list.push_back({ { "names"s, string(names) } });
		return list;
	}
}
